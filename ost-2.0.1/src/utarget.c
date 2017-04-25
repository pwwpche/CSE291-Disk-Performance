
/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By
 * downloading, copying, installing or using the software you agree to this
 * license. If you do not agree to this license, do not download, install, copy
 * or use the software. Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "device.h"
#include "config.h"

/*
 * Constants
 */

#define DFLT_ISCSI_LOGIN_SLEEP 0
#define DFLT_ADDRESS_FAMILY AF_INET


/*
 * Globals
 */

static int g_main_pid;
static int g_login_sleep = DFLT_ISCSI_LOGIN_SLEEP;
int g_port;
int g_family = DFLT_ADDRESS_FAMILY;

/* 
 * Control-C handler
 */

void handler(int s) {
    if (OST_GETPID != g_main_pid) return;
    if (target_shutdown()!=0) {
        TRACE_ERROR("target_shutdown() failed\n");
        return;
    }
    return;
}

void usage(char *argv[]) {
    printf("Target options:\n");
    printf("  -p <port>           Port number (dflt %i)\n", ISCSI_PORT);
    printf("  -F <family>         Address family (dflt %i)\n", 
           DFLT_ADDRESS_FAMILY);
    printf("  -s <secs>           "
           "Sleep <secs> before accepting new login (dflt %i)\n", 
           DFLT_ISCSI_LOGIN_SLEEP);
    printf("  -v                  Output version and exit\n");
    printf("  -h                  Output usage and exit\n");
    device_pargs();
}

int main(int argc, char *argv[]) {
    int port = ISCSI_PORT;
    char **argv_new;
    int argc_new = 0;
    int i;
    struct sigaction act;
    int fid; 
    char tmp[1024];

    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL); 


    //TRACE_ERROR("utarget started (pid %i, tid %li)\n", ISCSI_GETPID, ISCSI_GETTID);

    // Parse command line.  We remove args as we parse them and then pass
    // the new argc and argv to device_args().

    argv_new = malloc((argc+1)*sizeof(char *));
    for (i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-p")) {
            port = atoi(argv[i+1]); i++;
        } else if (!strcmp(argv[i], "-F")) {
            g_family = atoi(argv[i+1]); i++;
        } else if (!strcmp(argv[i], "-s")) {
            g_login_sleep = atoi(argv[i+1]); i++;
        } else if (!strcmp(argv[i], "-h")) {
            usage(argv);
            return 0;             
        } else if (!strcmp(argv[i], "-v")) {
            printf("%s\n", PACKAGE_STRING);
            return 0;             
        } else {
            argv_new[argc_new] = argv[i]; 
            argc_new++;
        }
    }

    // Parse device-specific command-line args

    if (device_args(argc_new, argv_new)!=0) {
        fprintf(stderr, "%s -h for usage\n", argv[0]);
        return -1;
    }
    free(argv_new);

    //usage(argv);

    signal (SIGINT, handler);
    g_main_pid = OST_GETPID;
    g_port = port;

    DEBUG_INIT;

    /* Check for already running (or failed) udisk */
    sprintf(tmp, "/tmp/UDISK.%i", g_port);
    if (open(tmp, O_RDONLY)!=-1) {
        fprintf(stderr, "/tmp/UDISK.%i exists -- udisk has failed or is already running\n", g_port);
        sleep(3600000);
	exit(1);
    }
    sprintf(tmp, "touch /tmp/UDISK.%i", g_port);
    system(tmp);

    /* Initialize target */
    if (target_init(1024)!=0) {
        TRACE_ERROR("target_init() failed\n");
        return -1;
    }


    /* Wait for connections */
    if (target_listen(port, g_login_sleep, g_family)!=0) {
        TRACE_ERROR("target_listen() failed\n");
    }

    DEBUG_SHUTDOWN;

    /* Remove lock file */
    sprintf(tmp, "rm /tmp/UDISK.%i", g_port);
    system(tmp);

    return 0;
}
