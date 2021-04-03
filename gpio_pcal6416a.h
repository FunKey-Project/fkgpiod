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
 *  @file gpio_pcal6416a.h
 *  This is userland GPIO driver for the PCAL6416AHB I2C GPIO expander chip
 */

#ifndef _GPIO_PCAL6416A_H_
#define _GPIO_PCAL6416A_H_


#include <stdbool.h>

/* Chip physical address */
#define PCAL6416A_I2C_ADDR              0x20
#define PCAL9539A_I2C_ADDR              0x76

/* Chip register adresses */
#define PCAL6416A_INPUT                 0x00 /* Input port [RO] */
#define PCAL6416A_DAT_OUT               0x02 /* GPIO DATA OUT [R/W] */
#define PCAL6416A_POLARITY              0x04 /* Polarity Inversion port [R/W] */
#define PCAL6416A_CONFIG                0x06 /* Configuration port [R/W] */
#define PCAL6416A_DRIVE0                0x40 /* Output drive strength Port0 [R/W] */
#define PCAL6416A_DRIVE1                0x42 /* Output drive strength Port1 [R/W] */
#define PCAL6416A_INPUT_LATCH           0x44 /* Port0 Input latch [R/W] */
#define PCAL6416A_EN_PULLUPDOWN         0x46 /* Port0 Pull-up/Pull-down enbl [R/W] */
#define PCAL6416A_SEL_PULLUPDOWN        0x48 /* Port0 Pull-up/Pull-down slct [R/W] */
#define PCAL6416A_INT_MASK              0x4A /* Interrupt mask [R/W] */
#define PCAL6416A_INT_STATUS            0x4C /* Interrupt status [RO] */
#define PCAL6416A_OUTPUT_CONFIG         0x4F /* Output port config [R/W] */

bool pcal6416a_init(void);
bool pcal6416a_deinit(void);
int pcal6416a_read_mask_interrupts(void);
int pcal6416a_read_mask_active_GPIOs(void);

#endif  //_GPIO_PCAL6416A_H_
