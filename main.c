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
 *  @file main.c
 *  This file contains the main function for the FunKey S GPIO daemon
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <getopt.h>
#include "to_log.h"
#include "daemon.h"
#include "uinput.h"
#include "gpio_mapping.h"

#define VERSION     "0.0.1"
#define PID_FILE    "/var/run/fkgpiod.pid"

/* Background daemon flag */
static bool daemon = false;

/* GPIO configuration file name */
static const char *config_file = "fkgpiod.conf";

/* Print usage information */
static void print_usage(void)
{
    printf("Usage: fkgpiod [options] [config_file]\n");
    printf("Options:\n");
    printf(" -d, -D, --daemonize                              Launch as a background daemon\n");
    printf(" -h, -H, --help                                   Print option help\n");
    printf(" -k, -K, --kill                                   Kill background daemon\n");
    printf(" -v, --version                                    Print version information\n");
}

/* Print version information */
static void print_version(void)
{
    printf("fkgpiod version "VERSION"\n");
    printf("FunKey S GPIO daemon\n\n");
    printf("Copyright (C) 2021, Vincent Buso <vincent.buso@funkey-project.com>,\n");
    printf("Copyright (C) 2021, Michel Stempin  <michel.stempin@funkey-project.com>,\n");
    printf("All rights reserved.\n");
    printf("Released under the GNU Lesser General Public License version 2.1 or later\n");
}

/* Parse command line options */
static void parse_options(int argc, char *argv[])
{
    struct option long_options[] = {
        {"daemonize", 0, NULL, 0},
        {"help", 0, NULL, 0},
        {"kill", 0, NULL, 0},
        {"version", 0, NULL, 0},
        {0, 0, NULL, 0}
    };
    int c, opt;

    while (true) {
        c = getopt_long(argc, argv, "dDhHkKvV", long_options, &opt);
        if (c == -1) {

            /* End of options */
            break;
        }
        if (c == 0) {

            /* Match long option names and convert them to short options */
            if (!strcmp(long_options[opt].name, "daemonize")) {
                c = 'd';
            } else if (!strcmp(long_options[opt].name, "help")) {
                c = 'h';
             } else if (!strcmp(long_options[opt].name, "kill")) {
                c = 'k';
            } else if (!strcmp(long_options[opt].name, "version")) {
                c = 'v';
            }
        }
        switch (c) {
        case 'd':
        case 'D':

            /* Daemonize */
            daemon = true;
            break;

        case 'h':
        case 'H':

            /* Help */
            print_usage();
            exit(EXIT_SUCCESS);

        case 'k':
        case 'K':

            /* Kill running daemon */
            kill_daemon(PID_FILE);
            exit(EXIT_SUCCESS);

        case 'v':
        case 'V':

            /* Version */
            print_version();
            exit(EXIT_SUCCESS);

        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }
    if (optind > argc) {
        print_usage();
        exit(EXIT_FAILURE);
    } else if (optind < argc) {

        /* Last argument is the GPIO confguration file name */
        config_file = strdup(argv[optind]);
    }
}

/* Entry point */
int main(int argc, char *argv[])
{
    mapping_list_t mapping_list;

    /* Parse command line options */
    parse_options(argc, argv);
    if (daemon) {

        /* Run as a background daemon, redirect all output to syslog */
        openlog("fkgpiod", LOG_PERROR | LOG_PID | LOG_NDELAY, LOG_DAEMON);
        to_log(&stdout);
        to_log(&stderr);
        daemonize("/", PID_FILE);
    }

    /* Initialize the uinput device */
    init_uinput();

    /* Initialize the GPIO mapping */
    init_gpio_mapping(config_file, &mapping_list);
    while (true) {

        /* Main loop to handle the GPIOs */
        handle_gpio_mapping(&mapping_list);
    }

    /* Deinitialize the GPIO mapping */
    deinit_gpio_mapping();

    /* Close the uinput device */
    close_uinput();
    if (daemon) {

        /* Close the syslog */
        closelog();
    }
   return EXIT_SUCCESS;
}
