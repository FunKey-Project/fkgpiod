/*
    Copyright (C) 2020-2021 Vincent Buso <vincent.buso@funkey-project.com>
    Copyright (C) 2021 Michel Stempin <michel.stempin@funkey-project.com>

    This file is part of the FunKey S GPIO keyboard daemon.

    This is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the GNU C Library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA.
*/

/**
 *  @file gpio_mapping.c
 *  This file contains the GPIO mapping functions
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gpio_utils.h"
#include "gpio_axp209.h"
#include "gpio_mapping.h"
#include "gpio_pcal6416a.h"
#include "mapping_list.h"
#include "parse_config.h"
#include "uinput.h"

//#define DEBUG_GPIO
//#define DEBUG_PERIODIC_CHECK
#define ERROR_GPIO

#ifdef DEBUG_GPIO
    #define FK_DEBUG(...) syslog(LOG_DEBUG, __VA_ARGS__);
#else
    #define FK_DEBUG(...)
#endif

#ifdef DEBUG_PERIODIC_CHECK
    #define FK_PERIODIC(...) syslog(LOG_DEBUG, __VA_ARGS__);
#else
    #define FK_PERIODIC(...)
#endif

#ifdef ERROR_GPIO
    #define FK_ERROR(...) syslog(LOG_ERR, __VA_ARGS__);
#else
    #define FK_ERROR(...)
#endif

#define FIFO_FILE               "/tmp/fkgpiod.fifo"

/* These defines force to perform a GPIO sanity check after a timeout.
 * If not declared, there will be no timeout and no periodical sanity check of
 * GPIO expander values
 */
//#define TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP     1
#define TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP  (30 * 1000)

/* Short Power Enable Key (PEK) duration in microseconds */
#define SHORT_PEK_PRESS_DURATION_US             (200 * 1000)

/* PCAL6416A I2C GPIO expander interrupt pin */
#define GPIO_PIN_I2C_EXPANDER_INTERRUPT         ((('B' - '@') << 4) + 3) // PB3

/* AXP209 I2C PMIC interrupt pin */
#define GPIO_PIN_AXP209_INTERRUPT               ((('B' - '@') << 4) + 5) // PB5

/* Pseudo-bitmask for the short PEK key press */
#define SHORT_PEK_PRESS_GPIO_MASK               (1 << 5)

/* Pseudo-bitmask for the NOE signal */
#define NOE_GPIO_MASK                           (1 << 10)

/* Shell command for shutdown upon receiving either long PEK or NOE signal */
#define SHELL_COMMAND_SHUTDOWN                  "powerdown schedule 0.1"

/* PCAL6416A/PCAL9539A I2C GPIO expander chip pseudo-file descriptor */
static int fd_pcal6416a;

/* AXP209 I2C PMIC pseudo-file descriptor */
static int fd_axp209;

/* FIFO pseudo-file descriptor */
static int fd_fifo;

/* Mask of monitored GPIOs */
static uint32_t monitored_gpio_mask;

/* Mask of current GPIOs */
static uint32_t current_gpio_mask;

/* Total bytes read from the FIFO */
static size_t total_bytes = 0;

/* FIFO buffer */
char fifo_buffer[256];

/* Search for the GPIO mask into the mapping and apply the required actions */
static void apply_mapping(mapping_list_t *list, uint32_t gpio_mask)
{
    mapping_t *mapping;

    /* Search the whole mapping sorted by decreasing simultaneous GPIO number
     * The whole mapping must be checked, as several GPIO combinations may be
     * active at the same time, and no longer matching combinations must be
     * deactivated
     */
    for (mapping = first_mapping(list); !last_mapping(list, mapping);
        mapping = next_mapping(mapping)) {
        if ((mapping->gpio_mask & gpio_mask) == mapping->gpio_mask)  {

            /* If the current GPIO mask contains the mapping GPIO mask */
            FK_DEBUG("Found matching mapping:\n");
#ifdef DEBUG_GPIO
            dump_mapping(mapping);
#endif // DEBUG_GPIO
            if (mapping->activated == false) {

                /* Activate mapping */
                mapping->activated = true;
                if (mapping->type == MAPPING_KEY) {

                    /* Send the key down event */
                    FK_DEBUG("\t--> Key press %d\n", mapping->value.keycode);
                    sendKey(mapping->value.keycode, 1);
                } else if (mapping->type == MAPPING_COMMAND) {

                    /* Execute the corresponding Shell command */
                    FK_DEBUG("\t--> Execute Shell command \"%s\"\n",
                    mapping->value.command);
                    system(mapping->value.command);
                }
            }

            /* Subtract the matching GPIOs from
             * the current GPIO mask and activate it
             */
            gpio_mask ^= mapping->gpio_mask;
        } else if (mapping->activated) {

            /* Non-matching activated mapping, deactivate it */
            FK_DEBUG("Found activated mapping:\n");
#ifdef DEBUG_GPIO
            dump_mapping(mapping);
#endif // DEBUG_GPIO
            mapping->activated = false;
            if (mapping->type == MAPPING_KEY) {

                /* Send the key up event */
                FK_DEBUG("\t--> Key release %d\n", mapping->value.keycode);
                sendKey(mapping->value.keycode, 0);
            }
        }
    }
}

/* Initialize the GPIO interrupt for the I2C expander chip */
static bool init_gpio_interrupt(int gpio, int *fd, const char *edge)
{
    FK_DEBUG("Initializing interrupt for GPIO P%c%d (%d)\n",
        (gpio / 16) + '@', gpio % 16, gpio);
    if (gpio_export(gpio) < 0) {
        return false;
    }

    /* Initializing the GPIO interrupt */
     if (*edge) {
        if (gpio_set_edge(gpio, edge) < 0) {
            return false;
        }
    }

    /* Open the GPIO pseudo-file */
    *fd = gpio_fd_open(gpio, O_RDONLY);
    FK_DEBUG("GPIO fd is: %d\n", *fd);
    if (*fd < 0) {
        return false;
    }
    return true;
}

/* Deinitialize the GPIO interrupt for the I2C expander chip */
static void deinit_gpio_interrupt(int fd)
{
    FK_DEBUG("DeInitializing interrupt for GPIO fd %d\n", fd);

    /* Close the GPIO pseudo-file */
    gpio_fd_close(fd);
}

/* Initialize the GPIO mapping */
bool init_gpio_mapping(const char *config_filename,
    mapping_list_t *mapping_list)
{
    init_mapping_list(mapping_list);

    /* Read the configuration file to get all valid GPIO mappings */
    if (parse_config_file(config_filename, mapping_list, &monitored_gpio_mask) ==
        false) {
        return false;
    }
#ifdef DEBUG_GPIO
    printf("\nGPIO Map:\n=========\n");
    dump_mapping_list(mapping_list);
#endif // DEBUG_GPIO

    /* Force the NOE GPIO to be an active GPIO as it is not in the mapping */
    monitored_gpio_mask |= NOE_GPIO_MASK;

    /* Clear the current GPIO mask */
    current_gpio_mask = 0;

    /* Initialize the PCAL5616AHF I2C GPIO expander chip */
    if (pcal6416a_init() == false) {
        return false;
    }

    /* Initialize the GPIO interrupt for the I2C GPIO expander chip */
    FK_DEBUG("Initialize interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
    init_gpio_interrupt(GPIO_PIN_I2C_EXPANDER_INTERRUPT, &fd_pcal6416a,
        "both");

    /* Initialize the AXP209 PMIC */
    if (axp209_init() == false) {
        return false;
    }

    /* Initialize the GPIO interrupt for the AXP209 chip */
    FK_DEBUG("Initialize interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
    init_gpio_interrupt(GPIO_PIN_AXP209_INTERRUPT, &fd_axp209, "");

    /* Create the FIFO pseudo-file if it does not exist */
    FK_DEBUG("Create the FIFO pseudo-file if it does not exist\n");
    if (mkfifo(FIFO_FILE, O_RDWR | 0640) < 0 && errno != EEXIST) {
        FK_ERROR("Cannot create the \"%s\" FIFO: %s\n", FIFO_FILE,
            strerror(errno));
        return false;
    }

    /* Open the FIFO pseudo-file */
    FK_DEBUG("Open the FIFO pseudo-file\n");
    fd_fifo = open(FIFO_FILE, O_RDWR | O_NONBLOCK);
    if (fd_fifo < 0) {
        FK_ERROR("Cannot open the \"%s\" FIFO: %s\n", FIFO_FILE,
            strerror(errno));
        return false;
    }

    /* Clear buffer */
    total_bytes = 0;
    return true;
}

/*  Deinitialize the GPIO mapping */
void deinit_gpio_mapping(void)
{
    /* Deinitialize the GPIO interrupt for the I2C GPIO expander chip */
    FK_DEBUG("DeInitiating interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
    deinit_gpio_interrupt(fd_pcal6416a);

    /* Deinitialize the I2C GPIO expander chip */
    pcal6416a_deinit();

    /* Deinitialize the GPIO interrupt for the AXP209 PMIC chip */
    FK_DEBUG("DeInitiating interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
    deinit_gpio_interrupt(fd_axp209);

    /* Deinitialize the AXP209 PMIC chip */
    axp209_deinit();

    /* Close the FIFO pseudo-file */
    FK_DEBUG("Close the FIFO pseudo-file \n");
    close(fd_fifo);
}

/* Handle the GPIO mapping (with interrupts) */
void handle_gpio_mapping(mapping_list_t *list)
{
    int result, gpio, int_status, max_fd, fd, val_int_bank_3;
    ssize_t read_bytes;
    fd_set read_fds, except_fds;
    uint32_t interrupt_mask, previous_gpio_mask;
    bool pcal6416a_interrupt = false;
    bool axp209_interrupt = false;
    bool forced_interrupt = false;
    char buffer[2], *next_line;
    mapping_t *mapping;

    /* Initialize masks */
    previous_gpio_mask = current_gpio_mask;
    current_gpio_mask = 0;

    /* Listen to FIFO read availability */
    FD_ZERO(&read_fds);
    FD_SET(fd_fifo, &read_fds);

    /* Listen to interrupt exceptions */
    FD_ZERO(&except_fds);
    FD_SET(fd_pcal6416a, &except_fds);
    FD_SET(fd_axp209, &except_fds);

    /* Compute the maximum file descriptor number */
    max_fd = (fd_pcal6416a > fd_axp209) ? fd_pcal6416a : fd_axp209;
    max_fd = (fd_fifo > max_fd) ? fd_fifo : max_fd;

#ifdef TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP

    /* Select with normal (short) timeout */
    struct timeval timeout = {0, TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP};
    result = select(max_fd + 1, &read_fds, NULL, &except_fds, &timeout);
#elif TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP

    /* Select with debug (slow) timeout */
    struct timeval timeout = {TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP, 0};
    result = select(max_fd + 1, &read_fds, NULL, &except_fds, &timeout);
#else

    /* Select with no timeout */
    result = select(max_fd + 1, &read_fds, NULL, &except_fds, NULL);
#endif
    if (result == 0) {

        /* Timeout case */
        FK_PERIODIC("Timeout, forcing sanity check\n");

        /* Timeout forces a "Found interrupt" event for sanity check */
        pcal6416a_interrupt = axp209_interrupt = forced_interrupt = true;
    } else if (result < 0) {

        /* Error case  */
        FK_ERROR("select: %s\n", strerror(errno));
        return;
    } else {

        /* Check if we received something from the FIFO */
        if (FD_ISSET(fd_fifo, &read_fds)) {
            while (true) {
                read_bytes = read(fd_fifo, &fifo_buffer[total_bytes],
                    sizeof (fifo_buffer) - 1);
                if (read_bytes > 0) {
                    total_bytes += (size_t) read_bytes;
                } else if (errno == EWOULDBLOCK) {

                    /* Done reading */
                    FK_DEBUG("Read %d bytes from FIFO: \"%.*s\"\n",
                        (int) total_bytes, (int) total_bytes, fifo_buffer);
                    if (strtok_r(fifo_buffer, "\r\n", &next_line) != NULL) {
                        FK_DEBUG("Parse line \"%s\"\n", fifo_buffer);
                        if (parse_config_line(fifo_buffer, list,
                            &monitored_gpio_mask) == false) {
                            FK_ERROR("Error while parsing line \"%s\"\n",
                                fifo_buffer);
                        }
                        total_bytes -= next_line - fifo_buffer;
                        if (total_bytes != 0) {
                            memmove(fifo_buffer, next_line, total_bytes);
                        }
                    }
                    break;
                } else {
                    FK_ERROR("Cannot read from FIFO: %s\n", strerror(errno));
                    return;
                }
            }
        }

        /* Check which file descriptor is available for read */
        for (fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &except_fds)) {

                /* Rewind file and dummy read the current GPIO value */
                lseek(fd, 0, SEEK_SET);
                if (read(fd, &buffer, 2) != 2) {
		    FK_ERROR("read: %s\n", strerror(errno));
                    break;
                }

                /* Check if the interrupt is from I2C GPIO expander or AXP209 */
                if (fd == fd_pcal6416a) {
                    FK_DEBUG("Found interrupt generated by PCAL6416AHF\r\n");
                    pcal6416a_interrupt = true;
                } else if (fd == fd_axp209) {
                    FK_DEBUG("Found interrupt generated by AXP209\r\n");
                    axp209_interrupt = true;
                }
            }
        }
    }

    /* Process the AXP209 interrupts, if any */
    if (axp209_interrupt) {
        if (forced_interrupt) {
            FK_PERIODIC("Processing forced AXP209 interrupt\n");
        } else {
            FK_DEBUG("Processing real AXP209 interrupt\n");
        }
        val_int_bank_3 = axp209_read_interrupt_bank_3();
        if (val_int_bank_3 < 0) {
            FK_DEBUG("Could not read AXP209 by I2C\n");
            return;
        }

        /* Proccess the Power Enable Key (PEK) short keypress */
        if (val_int_bank_3 & AXP209_INTERRUPT_PEK_SHORT_PRESS) {
            FK_DEBUG("AXP209 short PEK key press detected\n");
            mapping = find_mapping(list, SHORT_PEK_PRESS_GPIO_MASK);
            if (mapping != NULL) {
                FK_DEBUG("Found matching mapping:\n");
#ifdef DEBUG_GPIO
                dump_mapping(mapping);
#endif // DEBUG_GPIO
                if (mapping->type == MAPPING_KEY) {
                    FK_DEBUG("\t--> Key press and release %d\n",
                        mapping->value.keycode);
                    sendKey(mapping->value.keycode, 1);
                    usleep(SHORT_PEK_PRESS_DURATION_US);
                    sendKey(mapping->value.keycode, 0);
                } else if (mapping->type == MAPPING_COMMAND) {
                    FK_DEBUG("\t--> Execute Shell command \"%s\"\n",
                        mapping->value.command);
                    system(mapping->value.command);
                }
            }
            }

        /* Proccess the Power Enable Key (PEK) long keypress, the AXP209
         * will shutdown the system in 3s anyway
         */
        if (val_int_bank_3 & AXP209_INTERRUPT_PEK_LONG_PRESS) {
            FK_DEBUG("AXP209 long PEK key press detected\n");
            FK_DEBUG("\t--> Execute Shell command \"%s\"\n",
                SHELL_COMMAND_SHUTDOWN);
            system(SHELL_COMMAND_SHUTDOWN);
        }
    }

    /* Process the PCAL6416A interrupts, if any */
    if (pcal6416a_interrupt) {
        if (forced_interrupt) {
            FK_PERIODIC("Processing forced PCAL6416AHF interrupt\n");
        } else {
            FK_DEBUG("Processing real PCAL6416AHF interrupt\n");
        }

        /* Read the interrupt mask */
        int_status = pcal6416a_read_mask_interrupts();
        if (int_status < 0) {
            FK_DEBUG("Could not read PCAL6416A interrupt status by I2C\n");
            return;
        }
        interrupt_mask = (uint32_t) int_status;

        /* Read the GPIO mask */
        current_gpio_mask = pcal6416a_read_mask_active_GPIOs();
        if (current_gpio_mask < 0) {
            FK_DEBUG("Could not read PCAL6416A active GPIOS by I2C\n");
            return;
        }

        /* Keep only monitored GPIOS */
        interrupt_mask &= monitored_gpio_mask;
        current_gpio_mask &= monitored_gpio_mask;

        /* Invert the active low N_NOE GPIO signal */
        current_gpio_mask ^= (NOE_GPIO_MASK);

        /* Sanity check: if we missed an interrupt for some reason,
         * check if the GPIO value has changed and force it
         */
        for (gpio = 0; gpio < MAX_NUM_GPIO; gpio++) {
            if (interrupt_mask & (1 << gpio)) {

                /* Found the GPIO in the interrupt mask */
                FK_DEBUG("\t--> Interrupt GPIO: %d\n", gpio);
            } else if ((current_gpio_mask ^ previous_gpio_mask) & (1 << gpio)) {

                /* The GPIO is not in the interrupt mask, but has changed, force
                * it
                */
                FK_DEBUG("\t--> No interrupt (missed) but value has changed on GPIO: %d\n",
                gpio);
                interrupt_mask |= 1 << gpio;
            }
        }
        if (!interrupt_mask) {

            /* No change */
            return;
        }
        FK_DEBUG("current_gpio_mask 0x%04X interrupt_mask 0x%04X\n",
            current_gpio_mask, int_status);

        /* Proccess the N_OE signal from the magnetic Reed switch, the
         * AXP209 will shutdown the system in 3s anyway
         */
        if (interrupt_mask & NOE_GPIO_MASK) {
            FK_DEBUG("NOE detected\n");
            FK_DEBUG("\t--> Execute Shell command \"%s\"\n",
                SHELL_COMMAND_SHUTDOWN);
            interrupt_mask &= ~NOE_GPIO_MASK;
            system(SHELL_COMMAND_SHUTDOWN);
        }
    }

        /* Apply the mapping for the current gpio mask */
        apply_mapping(list, current_gpio_mask);
    return;
}
