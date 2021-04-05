/*
    Copyright (C) 2020-2021 Michel Stempin <michel.stempin@funkey-project.com>

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
 *  @file to_log.c
 *  This file contains the syslog redirection functions
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <syslog.h>

/* Ordered map of log priority level and string prefix */
static char const *priority[] = {
    "EMERG: ",
    "ALERT: ",
    "CRIT: ",
    "ERR: ",
    "WARNING: ",
    "NOTICE: ",
    "INFO: ",
    "DEBUG: "
};

/* Cookie I/O function to redirect file output to syslog */
static size_t writer(void *cookie, char const *data, size_t length)
{
    (void) cookie;
    int p = LOG_DEBUG, l;

    /* Look for a priority prefix, if any */
    do {
        l = strlen(priority[p]);
    } while (memcmp(data, priority[p], l) && --p >= 0);
    if (p < 0) {
        p = LOG_INFO;
    } else {
        data += l;
        length -= l;
    }

    /* Strip leading spaces */
    while (*data == ' ') {
        ++data;
        --length;
    }
    syslog(p, "%.*s", (int) length, data);
    return length;
}

/* Dummy Cookie IO function */
static int noop(void)
{
    return 0;
}

/* Cookie I/O function structure */
static cookie_io_functions_t log_fns = {
    (void *) noop, (void *) writer, (void *) noop, (void *) noop
};

/* Redirect given file output descriptor to syslog */
void to_log(FILE **pfp)
{
    //setvbuf(*pfp = fopencookie(NULL, "w", log_fns), NULL, _IOLBF, 0); //not working with muslc
    setvbuf(*pfp, NULL, _IOLBF, 0);
}
