/*
    Copyright (C) 2020-2021 Michel Stempin <michel.stempin@funkey-project.com>
    Mostly inspired from the Linux kernel lists

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
 *  @file mapping_list.c
 *  This file contains the mapping list handling functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "mapping_list.h"
#include "parse_config.h"
#include "uinput.h"

//#define DEBUG_MAPPING_LIST
#define ERROR_MAPPING_LIST

#ifdef DEBUG_MAPPING_LIST
    #define FK_DEBUG(...) syslog(LOG_DEBUG, __VA_ARGS__);
#else
    #define FK_DEBUG(...)
#endif

#ifdef ERROR_MAPPING_LIST
    #define FK_ERROR(...) syslog(LOG_ERR, __VA_ARGS__);
#else
    #define FK_ERROR(...)
#endif

/* Compute the byte offset of a structure member */
#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER)  __compiler_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)
#endif

/* Retrieve a typed pointer to the structure containing the given member */
#define container_of(ptr, type, member) ({ \
    void *__mptr = (void *)(ptr); \
    ((type *)(__mptr - offsetof(type, member))); })

/* Retrieve a typed list entry containing the given node as member */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/* Loop over the list nodes */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/* Loop over the list nodes backwards */
#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

/* Loop over the nodes of a list, the node may be destroyed safely during the
 * process
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/* Add a mapping between a previous and a next one */
static inline void list_add_between(struct mapping_list_t *new,
    struct mapping_list_t *prev, struct mapping_list_t *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/* Add a mapping to the mapping list head */
static inline void list_add(struct mapping_list_t *new,
    struct mapping_list_t *head)
{
    list_add_between(new, head, head->next);
}

/* Add a mapping to the mapping list tail */
static inline void list_add_tail(struct mapping_list_t *new,
    struct mapping_list_t *head)
{
    list_add_between(new, head->prev, head);
}

/* Delete a mapping between a previous and a next one */
static inline void list_delete_between(struct mapping_list_t *prev,
    struct mapping_list_t *next)
{
    next->prev = prev;
    prev->next = next;
}

/* Delete a mapping from the mapping list */
static inline void list_delete(struct mapping_list_t *entry)
{
    list_delete_between(entry->prev, entry->next);
}

/* Initalize a mapping list */
void init_mapping_list(mapping_list_t *list)
{
    list->next = list;
    list->prev = list;
}

/* Clear a mapping list */
void clear_mapping_list(mapping_list_t *list)
{
    struct mapping_list_t *p, *n;
    mapping_t *tmp;

    list_for_each_safe(p, n, list) {
        tmp = list_entry(p, mapping_t, mappings);
        list_delete(&tmp->mappings);
        if (tmp->type == MAPPING_COMMAND) {
            if (tmp->value.command != NULL) {
                free(tmp->value.command);
            }
        } else if (tmp->activated == true) {
	    sendKey(tmp->value.keycode, 0);
	}
        free(tmp);
    }
}

/* Get a type pointer on the first mapping in a mapping list */
mapping_t *first_mapping(mapping_list_t *list)
{
    return list_entry(list->next, mapping_t, mappings);
}

/* Get a type pointer on the next mapping in a mapping list */
mapping_t *next_mapping(mapping_t *mapping)
{
    return list_entry(mapping->mappings.next, __typeof__(*(mapping)), mappings);
}

/* Check if the mapping is the last in the mapping list */
bool last_mapping(const mapping_list_t *list, const mapping_t *mapping)
{
    return mapping->mappings.next->prev == list;
}

/* Allocated and insert a mapping in the mapping list */
bool insert_mapping(mapping_list_t *list, mapping_t *mapping)
{
    mapping_t *new_mapping, *next_mapping;
    struct mapping_list_t *cur;

    new_mapping = (mapping_t *) malloc(sizeof (mapping_t));
    if (new_mapping == NULL) {
        return false;
    }
    new_mapping->gpio_mask = mapping->gpio_mask;
    new_mapping->type = mapping->type;
    new_mapping->bit_count = mapping->bit_count;
    new_mapping->activated = mapping->activated;
    switch (mapping->type) {
        case MAPPING_COMMAND:
        new_mapping->value.command = strdup(mapping->value.command);
        break;

    case MAPPING_KEY:
        new_mapping->value.keycode = mapping->value.keycode;
        break;

    default:
        FK_ERROR("Unknown mapping type %d\n", mapping->type);
        return false;
    }

    /* Insert the mapping before any mapping with the same count of simultaneous
     * GPIOs, the list is thus kept in this order
     */
    list_for_each(cur, list) {
        next_mapping = list_entry(cur, mapping_t, mappings);
        if (next_mapping->bit_count <= mapping->bit_count) {
            list_add_between(&new_mapping->mappings, cur->prev, cur);
            return true;
        }
    }

    /* By default, the ne mapping is inserted at the head position */
    list_add(&new_mapping->mappings, list);
    return true;
}

/* Find a mapping in a mappining list with the exact same GPIO mask */
mapping_t *find_mapping(mapping_list_t *list, uint32_t gpio_mask)
{
    struct mapping_list_t *cur;
    mapping_t *mapping;

    list_for_each(cur, list) {
        mapping = list_entry(cur, mapping_t, mappings);
        if (mapping->gpio_mask == gpio_mask) {
            return mapping;
        }
    }
    return NULL;
}

/* Remove a mapping from the mapping list */
bool remove_mapping(mapping_list_t *list, mapping_t *mapping)
{
    struct mapping_list_t *p;
    mapping_t *tmp;

    list_for_each(p, list) {
        tmp = list_entry(p, mapping_t, mappings);
        if (tmp == mapping) {
            list_delete(&tmp->mappings);
            switch (tmp->type) {
                case MAPPING_COMMAND:
                if (tmp->value.command != NULL) {
                    free(tmp->value.command);
                }
                break;

            case MAPPING_KEY:
	        if (tmp->activated == true) {
		    sendKey(tmp->value.keycode, 0);
		}
                break;

            default:
                FK_ERROR("Unknown mapping type %d\n", tmp->type);
                return false;
            }
            free(tmp);
            return true;
        }
    }
   return false;
}

/* Dump a mapping */
void dump_mapping(mapping_t *mapping)
{
    int i;
    uint32_t gpio_mask;

    printf("mapping %p, prev %p, next %p\n", mapping, mapping->mappings.prev,
        mapping->mappings.next);
    printf("gpio_mask 0x%04X bit_count %d activated %s\n", mapping->gpio_mask,
        mapping->bit_count, mapping->activated ? "true" : "false");
    printf("button%s ", mapping->bit_count == 1 ? " " : "s");
    for (i = 0, gpio_mask = mapping->gpio_mask; i < MAX_NUM_GPIO;
        i++, gpio_mask >>= 1) {
        if (gpio_mask & 1) {
            printf("%s%s", gpio_name(i), gpio_mask == 1 ? "\n" : "+");
        }
    }
    switch (mapping->type) {
    case MAPPING_COMMAND:
        printf("command \"%s\"\n", mapping->value.command);
        break;

    case MAPPING_KEY:
        printf("keycode %s (%d)\n", keycode_name(mapping->value.keycode),
            mapping->value.keycode);
        break;

    default:
        FK_ERROR("Unknown mapping type %d\n", mapping->type);
        break;
    }
}

/* Dump a mapping list */
void dump_mapping_list(mapping_list_t *list)
{
    struct mapping_list_t *p;
    mapping_t *tmp;

    list_for_each(p, list) {
        tmp = list_entry(p, mapping_t, mappings);
        dump_mapping(tmp);
        printf("\n");
    }
}

/* Save a mapping */
void save_mapping(mapping_t *mapping)
{
  int i, length;
    uint32_t gpio_mask;

    printf("MAP ");
    for (i = 0, length = 0, gpio_mask = mapping->gpio_mask; i < MAX_NUM_GPIO;
        i++, gpio_mask >>= 1) {
        if (gpio_mask & 1) {
            printf("%s%s", gpio_name(i), gpio_mask == 1 ? " " : "+");
	    length += strlen(gpio_name(i)) + 1;
        }
    }
    for (i = 9 - length; i > 0; i--) {
        printf(" ");
    }
    switch (mapping->type) {
    case MAPPING_COMMAND:
        printf("TO COMMAND %s\n", mapping->value.command);
        break;

    case MAPPING_KEY:
        printf("TO KEY     %s\n", keycode_name(mapping->value.keycode));
        break;

    default:
        FK_ERROR("Unknown mapping type %d\n", mapping->type);
        break;
    }
}

/* Save a mapping list */
void save_mapping_list(mapping_list_t *list)
{
    struct mapping_list_t *p;
    mapping_t *tmp;

    printf("CLEAR\n");
    list_for_each_prev(p, list) {
        tmp = list_entry(p, mapping_t, mappings);
        save_mapping(tmp);
    }
}
