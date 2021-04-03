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
 *  @file gpio_axp209.h
 *  This is the userland GPIO driver for the AXP209 PMIC
 */

#ifndef _GPIO_AXP209_H_
#define _GPIO_AXP209_H_

#include <stdbool.h>

/* Chip physical address */
#define AXP209_I2C_ADDR                         0x34

/* Chip register adresses */
#define AXP209_REG_32H                          0x32
#define AXP209_REG_PEK_PARAMS                   0x36
#define AXP209_INTERRUPT_BANK_1_ENABLE          0x40
#define AXP209_INTERRUPT_BANK_1_STATUS          0x48
#define AXP209_INTERRUPT_BANK_2_ENABLE          0x41
#define AXP209_INTERRUPT_BANK_2_STATUS          0x49
#define AXP209_INTERRUPT_BANK_3_ENABLE          0x42
#define AXP209_INTERRUPT_BANK_3_STATUS          0x4A
#define AXP209_INTERRUPT_BANK_4_ENABLE          0x43
#define AXP209_INTERRUPT_BANK_4_STATUS          0x4B
#define AXP209_INTERRUPT_BANK_5_ENABLE          0x44
#define AXP209_INTERRUPT_BANK_5_STATUS          0x4C

/* Masks */
#define AXP209_INTERRUPT_PEK_SHORT_PRESS        0x02
#define AXP209_INTERRUPT_PEK_LONG_PRESS         0x01

bool axp209_init(void);
bool axp209_deinit(void);
int axp209_read_interrupt_bank_3(void);

#endif  //_GPIO_AXP209_H_
