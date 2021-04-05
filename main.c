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
    printf("Usage: fkgpiod [options] [config_file]\n"
           "Options:\n"
           " -d, -D, --daemonize                                Launch as a background daemon\n"
           " -h, -H, --help                                     Print option help\n"
           " -k, -K, --kill                                     Kill background daemon\n"
           " -v, --version                                      Print version information\n"
           "\n"
           "You can send script commands to the fkgpiod daemon by writting to the /tmp/fkgpiod.fifo file:\n"
           "\n"
           "$ echo \"LOAD /etc/fkgpiod.conf\" > /tmp/fkgpiod.fifo\n"
           "\n"
           "Available script commands (commands are not case sensitive):\n"
           "-----------------------------------------------------------\n"
           "DUMP                                                Dump the button mapping\n"
           "KEYDOWN <keycode>                                   Send a key down event with the given keycode\n"
           "KEYPRESS <keycode>                                  Send key press event with the given keycode\n"
           "KEYUP <keycode>                                     Send a key up event with the given keycode\n"
           "LOAD <configuration_file>                           Load a configuration file\n"
           "MAP <button_combination> TO KEY <keycode>           Map a button combination to a keycode\n"
           "MAP <button_combination> TO COMMAND <shell_command> Map a button combination to a Shell command\n"
           "RESET                                               Reset the button mapping\n"
           "SLEEP <delays_ms>                                   Sleep for the given delay in ms\n"
           "TYPE <string>                                       Type in a string\n"
           "UNMAP <button_combination>                          Unmap a button combination\n"
           "\n"
           "where:\n"
           " - <button_combination> is a list of UP, DOWN, LEFT, RIGHT, A, B, L, R, X, Y, MENU, START or FN\n"
           "   separated by \"+\" signs\n"
           " - <shell_command> is any valid Shell command with its arguments\n"
           " - <configuration_file> is the full path to a configurtion file\n"
           " - <delay_ms> is a delay in ms\n"
           " - <string> is a character string\n"
           " - <keycode> is among:\n"
           "   - KEY_0 to KEY_9, KEY_A to KEY_Z\n"
           "   - KEY_F1 to KEY_F24, KEY_KP0 to KEY_KP9, KEY_PROG1 to KEY_PROG4\n"
           "   - BTN_0 to BTN_9, BTN_A to BTN_C, BTN_X to BTN_Z, BTN_BASE2 to BTN_BASE6\n"
           "   - BTN_BACK, BTN_BASE, BTN_DEAD, BTN_EXTRA, BTN_FORWARD, BTN_GAMEPAD, BTN_JOYSTICK, BTN_LEFT,\n"
           "     BTN_MIDDLE, BTN_MISC, BTN_MODE, BTN_MOUSE, BTN_PINKIE, BTN_RIGHT, BTN_SELECT, BTN_SIDE,\n"
           "     BTN_START, BTN_TASK, BTN_THUMB, BTN_THUMB2, BTN_THUMBL, BTN_THUMBR, BTN_TL, BTN_TL2, \n"
           "     BTN_TOP, BTN_TOP2, BTN_TR, BTN_TR2, BTN_TRIGGER,\n"
           "   - KEY_102ND, KEY_AGAIN, KEY_ALTERASE, KEY_APOSTROPHE, KEY_BACK, KEY_BACKSLASH, KEY_BACKSPACE,\n"
           "     KEY_BASSBOOST, KEY_BATTERY, KEY_BLUETOOTH, KEY_BOOKMARKS, KEY_BRIGHTNESSDOWN,\n"
           "     KEY_BRIGHTNESSUP, KEY_BRIGHTNESS_CYCLE, KEY_BRIGHTNESS_ZERO, KEY_CALC, KEY_CAMERA,\n"
           "     KEY_CANCEL, KEY_CAPSLOCK, KEY_CHAT, KEY_CLOSE, KEY_CLOSECD, KEY_COFFEE, KEY_COMMA,\n"
           "     KEY_COMPOSE, KEY_COMPUTER, KEY_CONFIG, KEY_CONNECT, KEY_COPY, KEY_CUT, KEY_CYCLEWINDOWS,\n"
           "     KEY_DASHBOARD, KEY_DELETE, KEY_DELETEFILE, KEY_DIRECTION, KEY_DISPLAY_OFF, KEY_DOCUMENTS,\n"
           "     KEY_DOT, KEY_DOWN, KEY_EDIT, KEY_EJECTCD, KEY_EJECTCLOSECD, KEY_EMAIL, KEY_END, KEY_ENTER,\n"
           "     KEY_EQUAL, KEY_ESC, KEY_EXIT, KEY_FASTFORWARD, KEY_FILE, KEY_FINANCE, KEY_FIND,\n"
           "     KEY_FORWARD, KEY_FORWARDMAIL, KEY_FRONT, KEY_GRAVE, KEY_HANGEUL, KEY_HANGUEL, KEY_HANJA,\n"
           "     KEY_HELP, KEY_HENKAN, KEY_HIRAGANA, KEY_HOME, KEY_HOMEPAGE, KEY_HP, KEY_INSERT, KEY_ISO,\n"
           "     KEY_KATAKANA, KEY_KATAKANAHIRAGANA, KEY_KBDILLUMDOWN, KEY_KBDILLUMTOGGLE, KEY_KBDILLUMUP,\n"
           "     KEY_KPASTERISK,KEY_KPCOMMA, KEY_KPDOT, KEY_KPENTER, KEY_KPEQUAL, KEY_KPJPCOMMA,\n"
           "     KEY_KPLEFTPAREN, KEY_KPMINUS, KEY_KPPLUS, KEY_KPPLUSMINUS, KEY_KPRIGHTPAREN, KEY_KPSLASH,\n"
           "     KEY_LEFT, KEY_LEFTALT, KEY_LEFTBRACE, KEY_LEFTCTRL, KEY_LEFTMETA, KEY_LEFTSHIFT,\n"
           "     KEY_LINEFEED, KEY_MACRO, KEY_MAIL, KEY_MEDIA, KEY_MENU, KEY_MICMUTE, KEY_MINUS, KEY_MOVE,\n"
           "     KEY_MSDOS, KEY_MUHENKAN, KEY_MUTE, KEY_NEW, KEY_NEXTSONG, KEY_NUMLOCK, KEY_OPEN,\n"
           "     KEY_PAGEDOWN, KEY_PAGEUP, KEY_PASTE, KEY_PAUSE, KEY_PAUSECD, KEY_PHONE, KEY_PLAY,\n"
           "     KEY_PLAYCD, KEY_PLAYPAUSE, KEY_POWER, EY_PREVIOUSSONG, KEY_PRINT, KEY_PROPS, KEY_QUESTION,\n"
           "     KEY_RECORD, KEY_REDO, KEY_REFRESH, KEY_REPLY, KEY_REWIND, KEY_RFKILL, KEY_RIGHT,\n"
           "     KEY_RIGHTALT, KEY_RIGHTBRACE, KEY_RIGHTCTRL, KEY_RIGHTMETA, KEY_RIGHTSHIFT, KEY_RO,\n"
           "     KEY_SAVE, KEY_SCALE, KEY_SCREENLOCK, KEY_SCROLLDOWN, KEY_SCROLLLOCK, KEY_SCROLLUP,\n"
           "     KEY_SEARCH, KEY_SEMICOLON, KEY_SEND, KEY_SENDFILE, KEY_SETUP, KEY_SHOP, KEY_SLASH,\n"
           "     KEY_SLEEP, KEY_SOUND, KEY_SPACE, KEY_SPORT, KEY_STOP, KEY_STOPCD, KEY_SUSPEND,\n"
           "     KEY_SWITCHVIDEOMODE, KEY_SYSRQ, KEY_TAB, KEY_UNDO, KEY_UNKNOWN, KEY_UP, KEY_UWB,\n"
           "     KEY_VIDEO_NEXT, KEY_VIDEO_PREV, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_WAKEUP, KEY_WIMAX,\n"
           "     KEY_WLAN, KEY_WWW, KEY_XFER, KEY_YEN, KEY_ZENKAKUHANKAKU\n");
}

/* Print version information */
static void print_version(void)
{
    printf("fkgpiod version "VERSION"\n");
    printf("FunKey S GPIO daemon\n\n");
    printf("Copyright (C) 2020-2021, Vincent Buso <vincent.buso@funkey-project.com>,\n");
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
        printf("Func %s, line %d\n", __func__, __LINE__);
        openlog("fkgpiod", LOG_PERROR | LOG_PID | LOG_NDELAY, LOG_DAEMON);
        printf("Func %s, line %d\n", __func__, __LINE__);
        to_log(&stdout);
        printf("Func %s, line %d\n", __func__, __LINE__);
        to_log(&stderr);
        printf("Func %s, line %d\n", __func__, __LINE__);
        daemonize("/", PID_FILE);
        printf("Func %s, line %d\n", __func__, __LINE__);
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
