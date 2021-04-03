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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    #define LOG_DEBUG(...) printf(__VA_ARGS__);
#else
    #define LOG_DEBUG(...)
#endif

#ifdef DEBUG_PERIODIC_CHECK
    #define LOG_PERIODIC(...) printf(__VA_ARGS__);
#else
    #define LOG_PERIODIC(...)
#endif

#ifdef ERROR_GPIO
    #define LOG_ERROR(...) fprintf(stderr, "ERR: " __VA_ARGS__);
#else
    #define LOG_ERROR(...)
#endif

/* These defines force to perform a GPIO sanity check after a timeout.
 * If not declared, there will be no timeout and no periodical sanity check of
 * GPIO expander values
 */
//#define TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP     1
#define TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP  (30 * 1000)

#define SHORT_PEK_PRESS_DURATION_US             (200 * 1000)

#define GPIO_PIN_I2C_EXPANDER_INTERRUPT	        ((('B' - '@') << 4) + 3) // PB3
#define GPIO_PIN_AXP209_INTERRUPT               ((('B' - '@') << 4) + 5) // PB5

#define SHORT_PEK_PRESS_GPIO_MASK               (1 << 5)
#define NOE_GPIO_MASK                           (1 << 10)
#define SHELL_COMMAND_SHUTDOWN                  "sched_shutdown 0.1"

static int fd_pcal6416a;
static int fd_axp209;
static uint32_t active_gpio_mask;
static uint32_t current_gpio_mask;

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
            LOG_DEBUG("Found matching mapping:\n");
#ifdef DEBUG_GPIO
            dump_mapping(mapping);
#endif // DEBUG_GPIO
            if (mapping->activated == false) {

                /* Mapping is not yet active, subtract the matching GPIOs from
                 * the current GPIO mask and activate it
                 */
                gpio_mask ^= mapping->gpio_mask;
                mapping->activated = true;
                if (mapping->type == MAPPING_KEY) {

                    /* Send the key down event */
                    LOG_DEBUG("\t--> Key press %d\n", mapping->value.keycode);
                    sendKey(mapping->value.keycode, 1);
                } else if (mapping->type == MAPPING_COMMAND) {

                    /* Execute the corresponding Shell command */
                    LOG_DEBUG("\t--> Execute Shell command \"%s\"\n",
                    mapping->value.command);
                    system(mapping->value.command);
                }
            }
        } else if (mapping->activated) {

            /* Non-matching activated mapping, deactivate it */
            LOG_DEBUG("Found activated mapping:\n");
#ifdef DEBUG_GPIO
            dump_mapping(mapping);
#endif // DEBUG_GPIO
            mapping->activated = false;
            if (mapping->type == MAPPING_KEY) {

                /* Send the key up event */
                LOG_DEBUG("\t--> Key release %d\n", mapping->value.keycode);
                sendKey(mapping->value.keycode, 0);
            }
        }
    }
}

/* Initialize the GPIO interrupt for the I2C expander chip */
static bool init_gpio_interrupt(int gpio, int *fd, const char *edge)
{
    LOG_DEBUG("Initializing interrupt for GPIO P%c%d (%d)\n",
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
    LOG_DEBUG("GPIO fd is: %d\n", *fd);
    if (*fd < 0) {
        return false;
    }
    return true;
}

/* Deinitialize the GPIO interrupt for the I2C expander chip */
static void deinit_gpio_interrupt(int fd)
{
    LOG_DEBUG("DeInitializing interrupt for GPIO fd %d\n", fd);

    /* Close the GPIO pseudo-file */
    gpio_fd_close(fd);
}

/* Initialize the GPIO mapping */
bool init_gpio_mapping(const char *config_filename,
    mapping_list_t *mapping_list)
{
    init_mapping_list(mapping_list);

    /* Read the configuration file to get all valid GPIO mappings */
    if (parse_config_file(config_filename, mapping_list, &active_gpio_mask) ==
        false) {
        return false;
    }
#ifdef DEBUG_GPIO
    printf("\nGPIO Map:\n=========\n");
    dump_mapping_list(mapping_list);
#endif // DEBUG_GPIO

    /* Force the NOE GPIO to be an active GPIO as it is not in the mapping */
    active_gpio_mask |= NOE_GPIO_MASK;

    /* Clear the current GPIO mask */
    current_gpio_mask = 0;

    /* Initialize the PCAL5616AHF I2C GPIO expander chip */
    if (pcal6416a_init() == false) {
        return false;
    }

    /* Initialize the GPIO interrupt for the I2C GPIO expander chip */
    LOG_DEBUG("Initialize interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
    init_gpio_interrupt(GPIO_PIN_I2C_EXPANDER_INTERRUPT, &fd_pcal6416a,
        "both");

    /* Initialize the AXP209 PMIC */
    if (axp209_init() == false) {
        return false;
    }

    /* Initialize the GPIO interrupt for the AXP209 chip */
    LOG_DEBUG("Initialize interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
    init_gpio_interrupt(GPIO_PIN_AXP209_INTERRUPT, &fd_axp209, "");
    return true;
}

/*  Deinitialize the GPIO mapping */
void deinit_gpio_mapping(void)
{
    /* Deinitialize the GPIO interrupt for the I2C GPIO expander chip */
    LOG_DEBUG("DeInitiating interrupt for GPIO_PIN_I2C_EXPANDER_INTERRUPT\n");
    deinit_gpio_interrupt(fd_pcal6416a);

    /* Deinitialize the I2C GPIO expander chip */
    pcal6416a_deinit();

    /* Deinitialize the GPIO interrupt for the AXP209 PMIC chip */
    LOG_DEBUG("DeInitiating interrupt for GPIO_PIN_AXP209_INTERRUPT\n");
    deinit_gpio_interrupt(fd_axp209);

    /* Deinitialize the AXP209 PMIC chip */
    axp209_deinit();
}

/* Handle the GPIO mapping (with interrupts) */
void handle_gpio_mapping(mapping_list_t *list)
{
    int result, gpio, int_status, max_fd, fd, val_int_bank_3;
    fd_set fds;
    uint32_t interrupt_mask, previous_gpio_mask;
    bool pcal6416a_interrupt = false;
    bool axp209_interrupt = false;
    bool forced_interrupt = false;
    char buffer[2];
    mapping_t *mapping;

    /* Initialize masks */
    previous_gpio_mask = current_gpio_mask;
    current_gpio_mask = 0;

    /* Listen to interrupts */
    FD_ZERO(&fds);
    FD_SET(fd_pcal6416a, &fds);
    FD_SET(fd_axp209, &fds);
    max_fd = (fd_pcal6416a > fd_axp209) ? fd_pcal6416a : fd_axp209;
#ifdef TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP
    struct timeval timeout = {0, TIMEOUT_MICROSEC_SANITY_CHECK_GPIO_EXP};
    result = select(max_fd + 1, NULL, NULL, &fds, &timeout);
#elif TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP
    struct timeval timeout = {TIMEOUT_SEC_SANITY_CHECK_GPIO_EXP, 0};
    result = select(max_fd + 1, NULL, NULL, &fds, &timeout);
#else
    result = select(max_fd + 1, NULL, NULL, &fds, NULL);
#endif
    if (result == 0) {

        /* Timeout case */
        LOG_PERIODIC("Timeout, forcing sanity check\n");

        /* Timeout forces a "Found interrupt" event for sanity check */
        pcal6416a_interrupt = axp209_interrupt = forced_interrupt = true;
    } else if (result < 0) {

        /* Error case  */
        perror("select");
        return;
    } else {

        /* Check which file descriptor is available for read */
        for (fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &fds)) {

                /* Rewind file and dummy read the current GPIO value */
                lseek(fd, 0, SEEK_SET);
                if (read(fd, &buffer, 2) != 2) {
                    perror("read");
                    break;
                }

                /* Check if the interrupt is from I2C GPIO expander or AXP209 */
                if (fd == fd_pcal6416a) {
                    LOG_DEBUG("Found interrupt generated by PCAL6416AHF\r\n");
                    pcal6416a_interrupt = true;
                } else if (fd == fd_axp209) {
                    LOG_DEBUG("Found interrupt generated by AXP209\r\n");
                    axp209_interrupt = true;
                }
            }
        }
    }

    /* Process the AXP209 interrupts, if any */
    if (axp209_interrupt) {
        if (forced_interrupt) {
            LOG_PERIODIC("Processing forced AXP209 interrupt\n");
        } else {
            LOG_DEBUG("Processing real AXP209 interrupt\n");
        }
        val_int_bank_3 = axp209_read_interrupt_bank_3();
        if (val_int_bank_3 < 0) {
            LOG_DEBUG("Could not read AXP209 by I2C\n");
            return;
        }

        /* Proccess the Power Enable Key (PEK) short keypress */
        if (val_int_bank_3 & AXP209_INTERRUPT_PEK_SHORT_PRESS) {
            LOG_DEBUG("AXP209 short PEK key press detected\n");
            mapping = find_mapping(list, SHORT_PEK_PRESS_GPIO_MASK);
            if (mapping != NULL) {
                LOG_DEBUG("Found matching mapping:\n");
                dump_mapping(mapping);
                if (mapping->type == MAPPING_KEY) {
                    LOG_DEBUG("\t--> Key press and release %d\n",
                        mapping->value.keycode);
                    sendKey(mapping->value.keycode, 1);
                    usleep(SHORT_PEK_PRESS_DURATION_US);
                    sendKey(mapping->value.keycode, 0);
                } else if (mapping->type == MAPPING_COMMAND) {
                    LOG_DEBUG("\t--> Execute Shell command \"%s\"\n",
                        mapping->value.command);
                    system(mapping->value.command);
                }
            }
	    }

        /* Proccess the Power Enable Key (PEK) long keypress, the AXP209
         * will shutdown the system in 3s anyway
         */
        if (val_int_bank_3 & AXP209_INTERRUPT_PEK_LONG_PRESS) {
            LOG_DEBUG("AXP209 long PEK key press detected\n");
            LOG_DEBUG("\t--> Execute Shell command \"%s\"\n",
                SHELL_COMMAND_SHUTDOWN);
            system(SHELL_COMMAND_SHUTDOWN);
        }
    }

    /* Process the PCAL6416A interrupts, if any */
    if (pcal6416a_interrupt) {
        if (forced_interrupt) {
            LOG_PERIODIC("Processing forced PCAL6416AHF interrupt\n");
        } else {
            LOG_DEBUG("Processing real PCAL6416AHF interrupt\n");
        }

        /* Read the interrupt mask */
        int_status = pcal6416a_read_mask_interrupts();
        if (int_status < 0) {
            LOG_DEBUG("Could not read PCAL6416A interrupt status by I2C\n");
            return;
        }
        interrupt_mask = (uint32_t) int_status;

        /* Read the GPIO mask */
        current_gpio_mask = pcal6416a_read_mask_active_GPIOs();
        if (current_gpio_mask < 0) {
            LOG_DEBUG("Could not read PCAL6416A active GPIOS by I2C\n");
            return;
        }

        /* Keep only active GPIOS */
        interrupt_mask &= active_gpio_mask;
        current_gpio_mask &= active_gpio_mask;

        /* Invert the active low N_NOE GPIO signal */
        current_gpio_mask ^= (NOE_GPIO_MASK);

        /* Sanity check: if we missed an interrupt for some reason,
         * check if the GPIO value has changed and force it
         */
        for (gpio = 0; gpio < MAX_NUM_GPIO; gpio++) {
            if (interrupt_mask & (1 << gpio)) {

                /* Found the GPIO in the interrupt mask */
                LOG_DEBUG("\t--> Interrupt GPIO: %d\n", gpio);
            } else if ((current_gpio_mask ^ previous_gpio_mask) & (1 << gpio)) {

                /* The GPIO is not in the interrupt mask, but has changed, force
                * it
                */
                LOG_DEBUG("\t--> No interrupt (missed) but value has changed on GPIO: %d\n",
                gpio);
                interrupt_mask |= 1 << gpio;
            }
        }
        if (!interrupt_mask) {

            /* No change */
            return;
        }
        LOG_DEBUG("current_gpio_mask 0x%04X interrupt_mask 0x%04X\n",
            current_gpio_mask, int_status);

        /* Proccess the N_OE signal from the magnetic Reed switch, the
         * AXP209 will shutdown the system in 3s anyway
         */
        if (interrupt_mask & NOE_GPIO_MASK) {
            LOG_DEBUG("NOE detected\n");
            LOG_DEBUG("\t--> Execute Shell command \"%s\"\n",
                SHELL_COMMAND_SHUTDOWN);
            interrupt_mask &= ~NOE_GPIO_MASK;
            system(SHELL_COMMAND_SHUTDOWN);
        }
    }

	/* Apply the mapping for the current gpio mask */
	apply_mapping(list, current_gpio_mask);
    return;
}