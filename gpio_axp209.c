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
 *  @file gpio_axp209.c
 *  This is the userland GPIO driver for the AXP209 PMIC
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <assert.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "smbus.h"
#include "gpio_axp209.h"

//#define DEBUG_AXP209
#define ERROR_AXP209

#ifdef DEBUG_AXP209
	#define LOG_DEBUG(...) printf(__VA_ARGS__);
#else
	#define LOG_DEBUG(...)
#endif

#ifdef ERROR_AXP209
    #define LOG_ERROR(...) fprintf(stderr, "ERR: " __VA_ARGS__);
#else
    #define LOG_ERROR(...)
#endif

/* AXP209 I2C PMIC pseudo-file descriptor */
static int fd_axp209;

/* The I2C bus pseudo-file name */
static const char i2c0_sysfs_filename[] = "/dev/i2c-0";

/* Initialize the AXP209 PMIC chip */
bool axp209_init(void)
{
    /* Open the I2C bus pseudo-file */
    if ((fd_axp209 = open(i2c0_sysfs_filename,O_RDWR)) < 0) {
        LOG_ERROR("Failed to open the I2C bus %s", i2c0_sysfs_filename);
        return false;
    }

    /* Acquire the bus access for the AXP209 PMIC chip */
    if (ioctl(fd_axp209, I2C_SLAVE, AXP209_I2C_ADDR) < 0) {
        LOG_DEBUG("Failed to acquire bus access and/or talk to slave, trying to force it\n");
        if (ioctl(fd_axp209, I2C_SLAVE_FORCE, AXP209_I2C_ADDR) < 0) {
            LOG_ERROR("Failed to acquire FORCED bus access and/or talk to slave.\n");
            return false;
        }
    }

    /* Set PEK Long press delay to 2.5s */
    if (i2c_smbus_write_byte_data(fd_axp209, AXP209_REG_PEK_PARAMS, 0x9F) < 0) {
        LOG_ERROR("Cannot set AXP209 PEK Long press delay to 2.5s\n");
    }

    /* Set N_OE Shutdown delay to 3s*/
    if (i2c_smbus_write_byte_data(fd_axp209, AXP209_REG_32H, 0x47) < 0) {
        LOG_ERROR("Cannot set AXP209 N_OE Shutdown delay to 3s\n");
    }

    /* Enable only chosen interrupts (PEK short and long presses)*/
    if (i2c_smbus_write_byte_data(fd_axp209, AXP209_INTERRUPT_BANK_3_ENABLE,
        0x03) < 0) {
        LOG_ERROR("Cannot intiialize interrupt bank 3 for AXP209\n");
    }
    return true;
}

/* Deinitialize the AXP209 PMIC chip */
bool axp209_deinit(void)
{

    /* Close the I2C bus pseudo-file */
    close(fd_axp209);
    return true;
}

/* Read the AXP209 PMIC chip interrupt register bank 3 */
int axp209_read_interrupt_bank_3(void)
{
  int value, result;

    value = i2c_smbus_read_byte_data(fd_axp209, AXP209_INTERRUPT_BANK_3_STATUS);
    if (value  < 0) {
        return value;
    }

    /* Clear the interrupts */
    result = i2c_smbus_write_byte_data(fd_axp209, AXP209_INTERRUPT_BANK_3_STATUS, 0xFF);
    if (result < 0) {
        return result;
    }
    LOG_DEBUG("READ AXP209_INTERRUPT_BANK_3_STATUS: 0x%02X\n", value);
    return value & 0xFF;
}
