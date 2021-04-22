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
 *  @file gpio_pcal6416a.c
 *  This is userland GPIO driver for the PCAL6416AHB I2C GPIO expander chip
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "gpio_pcal6416a.h"
#include "smbus.h"

//#define DEBUG_PCAL6416A
#define ERROR_PCAL6416A

#ifdef DEBUG_PCAL6416A
    #define FK_DEBUG(...) syslog(LOG_DEBUG, __VA_ARGS__);
#else
        #define FK_DEBUG(...)
#endif

#ifdef ERROR_PCAL6416A
    #define FK_ERROR(...) syslog(LOG_ERR, __VA_ARGS__);
#else
    #define FK_ERROR(...)
#endif

/* Structure to map I2C address and I2C GPIO expander name */
typedef struct {
    unsigned int address;
    char *name;
} i2c_expander_t;

/* PCAL6416A/PCAL9539A I2C GPIO expander chip pseudo-file descriptor */
static int fd_i2c_expander;

/* The I2C bus pseudo-file name */
static char i2c0_sysfs_filename[] = "/dev/i2c-0";

/* PCAL6416A/PCAL9539A I2C GPIO expander chip I2C address */
static unsigned int i2c_expander_addr;

/* Map of I2C addresses / GPIO expander name */
static i2c_expander_t i2c_chip[] = {
    {PCAL9539A_I2C_ADDR, "PCAL9539A"},
    {PCAL6416A_I2C_ADDR, "PCAL6416A"},
    {0, NULL}
};

/* Initialize the PCAL6416A/PCAL9539A I2C GPIO expander chip */
bool pcal6416a_init(void)
{
    int i;

    /* Open the I2C bus pseudo-file */
    if ((fd_i2c_expander = open(i2c0_sysfs_filename,O_RDWR)) < 0) {
        FK_ERROR("Failed to open the I2C bus %s", i2c0_sysfs_filename);
        return false;
    }

    /* Probing known I2C GPIO expander chips */
    for (i = 0, i2c_expander_addr = 0; i2c_chip[i].address; i++) {

        if (ioctl(fd_i2c_expander, I2C_SLAVE_FORCE, i2c_chip[i]) < 0 ||
            pcal6416a_read_mask_interrupts() < 0) {
            FK_DEBUG("Failed to acquire bus access and/or talk to slave %s at address 0x%02X.\n",
                i2c_chip[i].name, i2c_chip[i].address);
        } else {
            FK_DEBUG("Found I2C gpio expander chip %s at address 0x%02X\n",
                i2c_chip[i].name, i2c_chip[i].address);
            i2c_expander_addr = i2c_chip[i].address;
            break;
        }
    }

    /* GPIO expander chip found? */
    if (!i2c_expander_addr) {
        FK_ERROR("Failed to acquire bus access and/or talk to slave, exit\n");
        return false;
    }
    i2c_smbus_write_word_data ( fd_i2c_expander, PCAL6416A_CONFIG, 0xffff);
    i2c_smbus_write_word_data ( fd_i2c_expander, PCAL6416A_INPUT_LATCH, 0x0000);
    i2c_smbus_write_word_data ( fd_i2c_expander, PCAL6416A_EN_PULLUPDOWN, 0xffff);
    i2c_smbus_write_word_data ( fd_i2c_expander, PCAL6416A_SEL_PULLUPDOWN, 0xffff);
    i2c_smbus_write_word_data ( fd_i2c_expander, PCAL6416A_INT_MASK, 0x0320);
    return true;
}

/* Deinitialize the PCAL6416A/PCAL9539A I2C GPIO expander chip */
bool pcal6416a_deinit(void)
{

    /* Close the I2C bus pseudo-file */
    close(fd_i2c_expander);
    return true;
}

/* Read the PCAL6416A/PCAL9539A I2C GPIO expander chip interrupt register */
int pcal6416a_read_mask_interrupts(void)
{
    int val_int;
    uint16_t val;

    val_int = i2c_smbus_read_word_data(fd_i2c_expander, PCAL6416A_INT_STATUS);
    if (val_int < 0) {
        return val_int;
    }
    val = val_int & 0xFFFF;
    FK_DEBUG("READ PCAL6416A_INT_STATUS :  0x%04X\n", val);
    return (int) val;
}

/* Read the PCAL6416A/PCAL9539A I2C GPIO expander chip active GPIO register */
int pcal6416a_read_mask_active_GPIOs(void)
{
    int val_int;
    uint16_t val;

    val_int = i2c_smbus_read_word_data(fd_i2c_expander, PCAL6416A_INPUT);
    if (val_int <  0){
        return val_int;
    }
    val = val_int & 0xFFFF;
    val = 0xFFFF - val;
    FK_DEBUG("READ PCAL6416A_INPUT (active GPIOs) :  0x%04X\n", val);
    return (int) val;
}
