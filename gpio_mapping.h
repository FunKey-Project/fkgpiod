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
 *  @file gpio_mapping.h
 *  This file contains the GPIO mapping functions
 */

#ifndef _GPIO_MAPPING_H_
#define _GPIO_MAPPING_H_

#include "mapping_list.h"

bool init_gpio_mapping(const char* config_filename,
    mapping_list_t *mapping_list);
void deinit_gpio_mapping(void);
void handle_gpio_mapping(mapping_list_t *mapping_list);

#endif	//_GPIO_MAPPING_H_
