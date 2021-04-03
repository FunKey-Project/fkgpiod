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
 *  @file gpio_utils.c
 *  This file contains the GPIO utility functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "gpio_utils.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

/* Export a GPIO in the sysfs pseudo-filesystem */
int gpio_export(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0) {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof (buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/* Unexport a GPIO in the sysfs pseudo-filesystem */
int gpio_unexport(unsigned int gpio)
{
    int fd, len;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0) {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof (buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

/* Set a GPIO direction in the sysfs pseudo-filesystem */
int gpio_set_dir(unsigned int gpio, const char* dir)
{
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof (buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("gpio/direction");
        return fd;
    }
    write(fd, dir, sizeof (dir)+1);
    close(fd);
    return 0;
}

/* Set a GPIO value in the sysfs pseudo-filesystem */
int gpio_set_value(unsigned int gpio, unsigned int value)
{
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("gpio/set-value");
        return fd;
    }
    if (value) {
        write(fd, "1", 2);
    } else {
        write(fd, "0", 2);
    }
    close(fd);
    return 0;
}

/* Get a GPIO value from the sysfs pseudo-filesystem */
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
    int fd;
    char buf[MAX_BUF];
    char ch;

    snprintf(buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        perror("gpio/get-value");
        return fd;
    }
    read(fd, &ch, 1);
    if (ch != '0') {
        *value = 1;
    } else {
        *value = 0;
    }
    close(fd);
    return 0;
}

/* Set a GPIO interrupt edge in the sysfs pseudo-filesystem, must be a string
 * among "none", "rising", "falling", or "both"
 */
int gpio_set_edge(unsigned int gpio, const char *edge)
{
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("gpio/set-edge");
        return fd;
    }
    write(fd, edge, strlen(edge) + 1);
    close(fd);
    return 0;
}

/* Open a GPIO pseudo-file from the sysfs pseudo-filesystem */
int gpio_fd_open(unsigned int gpio, unsigned int dir)
{
    int fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof (buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, dir | O_NONBLOCK );
    if (fd < 0) {
        perror("gpio/fd_open");
    }
    return fd;
}

/* Close a GPIO pseudo-file from the sysfs pseudo-filesystem */
int gpio_fd_close(int fd)
{
    return close(fd);
}
