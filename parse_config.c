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
 *  @file parse_config.c
 *  This file contains the configuration parsing functions
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "keydefs.h"
#include "mapping_list.h"
#include "parse_config.h"
#include "uinput.h"

//#define DEBUG_CONFIG
#define ERROR_CONFIG

#ifdef DEBUG_CONFIG
    #define LOG_DEBUG(...) printf(__VA_ARGS__);
#else
    #define LOG_DEBUG(...)
#endif

#ifdef ERROR_CONFIG
    #define LOG_ERROR(...) fprintf(stderr, "ERR: " __VA_ARGS__);
#else
    #define LOG_ERROR(...)
#endif

#define MAX_BUFFER_LENGTH 256
#define MAX_LINE_LENGTH 256

/* Structure to map command keywords and states */
typedef struct {
    const char *command;
    parse_state_t state;
} keyword_t;

#undef X
#define X(a, b) b,
//static const char *parse_state_names[] = {PARSE_STATES};

#undef X
#define X(a, b) b,
static const char *gpio_names[] = {GPIOS};

/* Map between command keywords and states */
static const keyword_t valid_commands[] = {
    {"MAP", STATE_MAP},
    {"UNMAP", STATE_UNMAP},
    {"RESET", STATE_RESET},
    {"LOAD", STATE_LOAD},
    {"SLEEP", STATE_SLEEP},
    {"KEYUP", STATE_KEYUP},
    {"KEYDOWN", STATE_KEYDOWN},
    {"KEYPRESS", STATE_KEYPRESS},
    {"TYPE", STATE_TYPE},
    {"HELP", STATE_HELP},
    {"", STATE_INVALID}
};

/* Map between function keywords and states */
static const keyword_t valid_functions[] = {
    {"KEY", STATE_KEY},
    {"COMMAND", STATE_COMMAND},
    {"", STATE_INVALID}
};

/* The command keywor state */
static parse_state_t keyword;

/* Buffer for argument parsing */
static char buffer[MAX_BUFFER_LENGTH + 1];

/* Line buffer for parsing */
static char line[MAX_LINE_LENGTH + 1];

/* Lookup a command parse state from a token */
static parse_state_t lookup_command(char *token)
{
    int command;

    for (command = 0; valid_commands[command].state != STATE_INVALID;
        command++) {
        if (strcasecmp(token, valid_commands[command].command) == 0) {
            keyword = valid_commands[command].state;
            return keyword;
        }
    }
    LOG_ERROR("Invalid keyword \"%s\"\n", token);
    return STATE_INVALID;
}

/* Lookup a function parse state from a token */
static parse_state_t lookup_function(char *token)
{
    int command;

    for (command = 0; valid_functions[command].state != STATE_INVALID;
        command++) {
        if (strcasecmp(token, valid_functions[command].command) == 0) {
            return valid_functions[command].state;
        }
    }
    LOG_ERROR("Invalid keyword \"%s\"\n", token);
    return STATE_INVALID;
}

/* Lookup a GPIO number from a token */
static int lookup_gpio(char *token)
{
    int button;

    for (button = 0; gpio_names[button] != NULL; button++) {
        if (strcasecmp(token, gpio_names[button]) == 0) {
            LOG_DEBUG("Found button \"%s\" (GPIO %d)\n", gpio_names[button],
                button);
            return button;
        }
    }
    LOG_ERROR("Unknown button \"%s\"\n", token);
    return -1;
}

/* Lookup a key code from a token */
static int lookup_key(char *token)
{
    int key;

    for (key = 0; key_names[key].code >= 0; key++) {
        if (strcasecmp(token, key_names[key].name) == 0) {
            LOG_DEBUG("Found keycode \"%s\" (%d)\n", key_names[key].name,
                key_names[key].code);
            return key;
        }
    }
    LOG_ERROR("Unknown key \"%s\"\n", token);
    return -1;
}

/* Get a GPIO name */
const char *gpio_name(uint8_t gpio)
{
    if (gpio < GPIO_LAST) {
        return gpio_names[gpio];
    }
    return "?";
}

/* Get a keycode name */
const char *keycode_name(int keycode)
{
    int key;

    for (key = 0; key_names[key].code >= 0; key++) {
        if (key_names[key].code == keycode) {
            return key_names[key].name;
        }
    }
    return "?";
}

/* Parse a configuration line */
bool parse_config_line(char *line, mapping_list_t *list,
    uint32_t *active_gpio_mask)
{
    int button_count = 0, button, key = 0;
    parse_state_t state = STATE_INIT;
    char *token, *next_token, *token_end = NULL, *s;
    bool expecting_button = true;
    uint32_t gpio_mask = 0;
    mapping_t *existing_mapping, new_mapping;

    token = strtok_r(line, " \t", &next_token);
    while (token != NULL) {
        switch (state) {
        case STATE_INIT:
            state = lookup_command(token);
            if (state == STATE_INVALID) {
                return false;
            }
            break;

        case STATE_UNMAP:
        case STATE_MAP:
            if (strcasecmp(token, "TO") == 0) {
                if (state != STATE_MAP) {
                    LOG_ERROR("Unexpected keyword \"TO\"\n");
                    return false;
                }
                if (expecting_button == true) {

                    /* Missing last button */
                    LOG_ERROR("Missing last button\n");
                    return false;
                } else if (button_count == 0) {

                    /* No button found */
                    LOG_ERROR("No button found\n");
                    return false;
                }
                state = STATE_FUNCTION;
                break;
            }
            do {
                token_end = strchr(token, '+');
                if (token_end != NULL) {
                    if (token_end == token) {

                        /* Leading '+' */
                        if (button_count == 0) {

                            /* We need at least one button left of '+' */
                            LOG_ERROR("Missing first button\n");
                            return false;
                        } else if (expecting_button == true) {

                            /* Exepecting a button, cannot have another '+' */
                            LOG_ERROR("Missing button\n");
                            return false;
                        } else {

                            /* Skip leading '+' */
                            token++;
                            expecting_button = true;
                            continue;
                        }
                    }

                    /* Composite button definition, lookup buttons one by one */
                    *token_end = '\0';
                }

                /* GPIO lookup */
                if (*token == '\0') {
                    break;
                }
                if ((button = lookup_gpio(token)) >= 0) {
                    button_count++;
                    expecting_button = false;
                    gpio_mask |= 1 << button;
                } else {
                    return false;
                }
                if (token_end != NULL) {
                    token = token_end + 1;
                    expecting_button = true;
                }
            } while (token_end != NULL);
            break;

        case STATE_RESET:
            break;

        case STATE_LOAD:
            state = STATE_FILE;
            break;

        case STATE_SLEEP:
            state = STATE_DELAY;
            break;

        case STATE_KEYUP:
        case STATE_KEYDOWN:
        case STATE_KEYPRESS:
            state = STATE_KEY;
            break;

        case STATE_TYPE:
            state = STATE_TEXT;
            break;

        case STATE_DELAY:
            for (s = token; *s; s++) {
                if (!isdigit(*s)) {
                    LOG_ERROR("Invalid delay \"%s\"\n", token);
                    return false;
                }
            }
        case STATE_FILE:
        case STATE_TEXT:
            strncpy(buffer, token, MAX_LINE_LENGTH);
            break;

        case STATE_FUNCTION:
            state = lookup_function(token);
            if (state == STATE_INVALID) {
                return false;
            }
            buffer[0] = '\0';
            break;

        case STATE_KEY:
            if ((key = lookup_key(token)) >= 0) {
                break;
            } else {
                return false;
            }
            break;

        case STATE_COMMAND:
            if (buffer[0] != '\0') {
                strncat(buffer, " ", MAX_BUFFER_LENGTH);
            }
            strncat(buffer, token, MAX_BUFFER_LENGTH);
            break;

        default:
            LOG_ERROR("Unknown state %d\n", state);
            return false;
        }
        token = strtok_r(NULL, " \t", &next_token);
    }
    switch (state) {
    case STATE_UNMAP:
        LOG_DEBUG("UNMAP gpio_mask 0x%04X button_count %d\n", gpio_mask,
            button_count);
        existing_mapping = find_mapping(list, gpio_mask);
        if (existing_mapping == NULL) {
            LOG_ERROR("Cannot find mapping with gpio_mask 0x%04X\n",
                gpio_mask);
            return false;
        }
        if (remove_mapping(list, existing_mapping) == false) {
            LOG_ERROR("Cannot remove mapping with gpio_mask 0x%04X\n",
                gpio_mask);
            return false;
        }
        *active_gpio_mask &= ~gpio_mask;
        break;

    case STATE_RESET:
        LOG_DEBUG("RESET\n");
        reset_mapping_list(list);
        break;

    case STATE_FILE:
        LOG_DEBUG("LOAD file \"%s\"\n", buffer);
        return parse_config_file(buffer, list, active_gpio_mask);
        break;

    case STATE_DELAY:
        LOG_DEBUG("SLEEP delay %s ms\n", buffer);
        usleep(atoi(buffer) * 1000);
        break;

    case STATE_TEXT:
        break;

    case STATE_KEY:
        switch (keyword) {
        case STATE_KEYUP:
            sendKey(key, 0);
            break;

        case STATE_KEYDOWN:
            sendKey(key, 1);
            break;

        case STATE_KEYPRESS:
            sendKey(key, 1);
            usleep(200 * 1000);
            sendKey(key, 0);
            break;

        case STATE_MAP:
            LOG_DEBUG("MAP gpio_mask 0x%04X to key %d, button_count %d\n",
                gpio_mask, key, button_count);
            existing_mapping = find_mapping(list, gpio_mask);
            if (existing_mapping != NULL) {
                LOG_DEBUG("Existing mapping with gpio_mask 0x%04X found\n",
                    gpio_mask);
                if (remove_mapping(list, existing_mapping) == false) {
                    LOG_ERROR("Cannot remove mapping with gpio_mask 0x%04X\n",
                        gpio_mask);
                    return false;
                }
            }
            new_mapping.gpio_mask = gpio_mask;
            new_mapping.bit_count = button_count;
            new_mapping.activated = false;
            new_mapping.type = MAPPING_KEY;
            new_mapping.value.keycode = key;
            if (insert_mapping(list, &new_mapping) == false) {
                LOG_ERROR("Cannot add mapping with gpio_mask 0x%04X\n",
                    gpio_mask);
                return false;
            }
            *active_gpio_mask |= gpio_mask;
            break;

        default:
            LOG_ERROR("Invalid keyword %d\n", keyword);
            return false;
        }
        break;

    case STATE_COMMAND:
        LOG_DEBUG("MAP gpio_mask 0x%04X to command \"%s\", button_count %d\n",
            gpio_mask, buffer, button_count);
        LOG_DEBUG("MAP gpio_mask 0x%04X to key %d, button_count %d\n",
            gpio_mask, key, button_count);
        existing_mapping = find_mapping(list, gpio_mask);
        if (existing_mapping != NULL) {
            LOG_DEBUG("Existing mapping with gpio_mask 0x%04X found\n",
                gpio_mask);
            if (remove_mapping(list, existing_mapping) == false) {
                LOG_ERROR("Cannot remove mapping with gpio_mask 0x%04X\n",
                    gpio_mask);
                return false;
            }
        }
        new_mapping.gpio_mask = gpio_mask;
        new_mapping.bit_count = button_count;
        new_mapping.activated = false;
        new_mapping.type = MAPPING_COMMAND;
        new_mapping.value.command = buffer;
        if (insert_mapping(list, &new_mapping) == false) {
            LOG_ERROR("Cannot add mapping with gpio_mask 0x%04X\n",
                gpio_mask);
            return false;
        }
        break;

    case STATE_MAP:
    case STATE_LOAD:
    case STATE_SLEEP:
    case STATE_TYPE:
    case STATE_HELP:
       break;

    default:
        LOG_ERROR("Unknown result state %d\n", state);
        return false;
    }
    return true;
}

/* Parse a configuration file */
bool parse_config_file(const char *name, mapping_list_t *list,
    uint32_t *active_gpio_mask)
{
    FILE *fp;
    int line_number = 0;

    if ((fp = fopen(name, "r")) == NULL) {
        LOG_ERROR("Cannot open file \"%s\"\n", name);
        return false;
    }
    while (!feof(fp)) {
        if (fgets(line, MAX_LINE_LENGTH, fp) != line) {
            if (!feof(fp)) {
                LOG_ERROR("Error reading file \"%s\": %s\n", name,
                    strerror(errno));
                fclose(fp);
                return false;
            }
        }
        line_number++;
        if (line[0] == '#') {

            /* Skip comment lines */
            continue;
        }

        /* Remove trailing CR/LF */
        strtok(line, "\r\n");

        /* Parse a configuration line */
        if (parse_config_line(line, list, active_gpio_mask) == false) {
            LOG_ERROR("line %d\n", line_number);
            break;
        }
    }
    fclose(fp);
    return true;
}
