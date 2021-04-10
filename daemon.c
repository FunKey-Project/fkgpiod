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
 *  @file daemon.c
 *  This file contains the daemon function for the FunKey S GPIO daemon
 */

#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "daemon.h"

/* Process ID file descriptor */
static int pid_fd;

/* Process ID lock file name */
static char *pid_lock_file;

/* Signal handler */
void signal_handler(int sig)
{
    switch(sig){
    case SIGHUP:
        syslog(LOG_WARNING, "Received SIGHUP.");
    break;

    case SIGINT:
    case SIGTERM:
        syslog(LOG_INFO, "Exiting.");
        close(pid_fd);
        unlink(pid_lock_file);
        exit(EXIT_SUCCESS);
        break;

    default:
        syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
    }
}

/* Get the pid from the pid lock file and terminate the daemon */
void kill_daemon(char *pidfile)
{
    int n;
    pid_t pid;
    char str[10];

    pid_fd = open(pidfile, O_RDONLY, 0600);
    if (pid_fd < 0) {
        perror(pidfile);
    } else {
        if (read(pid_fd, str, 10) > 0) {
            pid = strtol(str, NULL, 0);
            if (pid) {
                printf("terminating %d\n", pid);
                n = kill(pid, SIGTERM);
                if (n < 0) {
                    perror("kill");
                }
            }
        }
        close(pid_fd);
    }
}

/* Turn the running process into a daemon */
void daemonize(const char *ident, char *rundir, char *pidfile)
{
    int pid, i;
    char str[10];
    struct sigaction sa;
    sigset_t ss;

    /* Check if parent process is already init */
    if (getppid() == 1) {

        /* Don't need to do it twice */
        return;
    }

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        perror("first fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {

        /* Success. terminate the parent */
        exit(EXIT_SUCCESS);
    }

    /* On success: the child process becomes the session leader */
    if (setsid() < 0) {
        perror("set SID");
        exit(EXIT_FAILURE);
    }

    /* Block these signals */
    sigemptyset(&ss);
    sigaddset(&ss, SIGCHLD);
    sigaddset(&ss, SIGTSTP);
    sigaddset(&ss, SIGTTOU);
    sigaddset(&ss, SIGTTIN);
    sigprocmask(SIG_BLOCK, &ss, NULL);

    /* Handle these signals */
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    /* Fork off for a second time to ensure that we get rid of the session
     * leading process
     */
    pid = fork();
    if (pid < 0) {
        perror("second fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {

        /* Success. terminate the parent */
        exit(EXIT_SUCCESS);
    }

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory or another
     * appropriated directory
     */
    if (chdir(rundir) < 0) {
        perror(rundir);
    }

    /* Close all opened file descriptors */
    for (i = sysconf(_SC_OPEN_MAX); i >= 0; i--) {
        close(i);
    }

    /* Since we have no more stdout/stderr, open syslog */
    openlog(ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    /* Make sure there is only one daemon running at a time using a lock file */
    pid_lock_file = pidfile;
    pid_fd = open(pidfile, O_RDWR | O_CREAT, 0600);
    if (pid_fd < 0) {
        syslog(LOG_INFO, "Could not open lock file %s. Exiting.", pidfile);
        exit(EXIT_FAILURE);
    }
    if (lockf(pid_fd, F_TLOCK, 0) < 0) {
        syslog(LOG_INFO, "Could not lock lock file %s. Exiting.", pidfile);
        unlink(pidfile);
        exit(EXIT_FAILURE);
    }
    sprintf(str, "%d\n", getpid());
    if (write(pid_fd, str, strlen(str)) < 0) {
        syslog(LOG_ERR, "Could not write to lock file %s: %s. Exiting.",
            strerror(errno), pidfile);
        unlink(pidfile);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "Daemon running.");
}
