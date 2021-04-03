/*
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
 *  @file parse_config.h
 *  This file contains the configuration parsing functions
 */

#ifndef _PARSE_H_
#define _PARSE_H_

#include <stdint.h>
#include <stdbool.h>
#include "mapping_list.h"

/* Definition of the different hardware GPIOs */
#define GPIOS \
    X(GPIO_RIGHT, "RIGHT") \
    X(GPIO_DOWN, "DOWN") \
    X(GPIO_L, "L") \
    X(GPIO_UP, "UP") \
    X(GPIO_LEFT, "LEFT") \
    X(GPIO_MENU, "MENU") \
    X(GPIO_START, "START") \
    X(GPIO_FN, "FN") \
    X(GPIO_NU1, "") \
    X(GPIO_NU2, "") \
    X(GPIO_NU3, "") \
    X(GPIO_X, "X") \
    X(GPIO_A, "A") \
    X(GPIO_Y, "Y") \
    X(GPIO_B, "B") \
    X(GPIO_R, "R") \
    X(GPIO_LAST, NULL)

/* Enumeration of the different hardware GPIOs */
#undef X
#define X(a, b) a,
typedef enum {GPIOS} button_t;

/* Definition of the different parse states */
#define PARSE_STATES \
    X(STATE_INIT, "INIT") \
    X(STATE_MAP, "MAP") \
    X(STATE_UNMAP, "UNMAP") \
    X(STATE_RESET, "RESET") \
    X(STATE_LOAD, "LOAD") \
    X(STATE_FILE, "FILE") \
    X(STATE_SLEEP, "SLEEP") \
    X(STATE_DELAY, "DELAY") \
    X(STATE_KEYUP, "KEYUP") \
    X(STATE_KEYDOWN, "KEYDOWN") \
    X(STATE_KEYPRESS, "KEYPRESS") \
    X(STATE_TYPE, "TYPE") \
    X(STATE_TEXT, "TEXT") \
    X(STATE_FUNCTION, "FUNCTION") \
    X(STATE_KEY, "KEY") \
    X(STATE_COMMAND, "COMMAND")\
    X(STATE_HELP, "HELP") \
    X(STATE_INVALID, "INVALID")

/* Enumeration of the different parse states */
#undef X
#define X(a, b) a,
typedef enum {PARSE_STATES} parse_state_t;

const char *gpio_name(uint8_t gpio);
const char *keycode_name(int keycode);
bool parse_config_line(char *line, mapping_list_t *list,
    uint32_t *active_gpio_mask);
bool parse_config_file(const char *name, mapping_list_t *list,
    uint32_t *active_gpio_mask);

#endif //_PARSE_H_
