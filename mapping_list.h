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
 *  @file mapping_list.h
 *  This file contains the mapping list handling functions
 */

#ifndef _MAPPING_LIST_H_
#define _MAPPING_LIST_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_NUM_GPIO    32

/* Definition of the different mapping types */
#define MAPPING_TYPES \
    X(MAPPING_KEY, "KEY") \
    X(MAPPING_COMMAND, "COMMAND")

/* Enumeration of the different mapping types */
#undef X
#define X(a, b) a,
typedef enum {MAPPING_TYPES} mapping_type_t;

typedef struct mapping_list_t {
    struct mapping_list_t *next, *prev;
} mapping_list_t;

typedef struct mapping_t {
    struct mapping_list_t mappings;
    uint32_t gpio_mask;
    mapping_type_t type;
    union {
        char *command;
        int keycode;
    } value;
    uint8_t bit_count;
    bool activated;
} mapping_t;

void init_mapping_list(mapping_list_t *list);
void clear_mapping_list(mapping_list_t *list);
mapping_t *first_mapping(mapping_list_t *list);
mapping_t *next_mapping(mapping_t *mapping);
bool last_mapping(const mapping_list_t *list, const mapping_t *mapping);
bool insert_mapping(mapping_list_t *list, mapping_t *mapping);
mapping_t *find_mapping(mapping_list_t *list, uint32_t gpio_mask);
bool remove_mapping(mapping_list_t *list, mapping_t *mapping);
void dump_mapping(mapping_t *mapping);
void dump_mapping_list(mapping_list_t *list);

#endif // _MAPPING_LIST_H_
