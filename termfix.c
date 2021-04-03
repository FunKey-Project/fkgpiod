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
 *  @file termfix.c
 *  This file contains a function to unlock and force a vt back into text mode
 */

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <linux/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage: termfix /dev/ttyX\n");
        return 2;
    }

    int fd = open(argv[1], O_RDWR, 0);
    int res = ioctl(fd, VT_UNLOCKSWITCH, 1);

    if (res != 0) {
        perror("ioctl VT_UNLOCKSWITCH failed");
        return 3;
    }

    ioctl(fd, KDSETMODE, KD_TEXT);

    if (res != 0) {
        perror("ioctl KDSETMODE failed");
        return 3;
    }

    printf("Success\n");

    return res;
}
