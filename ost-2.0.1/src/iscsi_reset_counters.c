/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <config.h>

int all = 0;
int lba_a = 0; 
int lba_b = 0; 
int dump = 0;
char procfile[64] = "";

void usage(char *argv[]) {
    printf("usage: %s\n", argv[0]);
    printf("       --file <file>     -- proc file (e.g., /proc/scsi/iscsi/2)\n");
    printf("       --all             -- reset all driver counters\n");
    printf("       --dump            -- prepare to dump I/O traces\n");
    printf("       --lba_a           -- reset min/max lba (cookie a)\n");
    printf("       --lba_b           -- reset min/max lba (cookie b)\n");
    printf("       --help            -- show usage\n");
}

int main(int argc, char *argv[]) {
    int option_index = 0;
    int digit_optind = 0;
    char c;
    int fid;
    int rc = 0;

    while (1) {
	int this_option_optind = optind ? optind : 1;
	
	static struct option long_options[] = {
            {"file", 1, 0, 'f'},
            {"all", 0, &all, 1},
            {"dump", 0, &dump, 1},
            {"lba_a", 0, &lba_a, 1},
            {"lba_b", 0, &lba_b, 1},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
	    {0, 0, 0, 0}
	};
        next: c = getopt_long (argc, argv, "uvf:", long_options, &option_index);
        if (c == 0) goto next;
        if (c == -1) break;
        switch (c) {
        case 'f':
            strcpy(procfile, optarg);
            break;
        case 'h':
            usage(argv);
            return 0;
        case 'v':
            printf("%s\n", PACKAGE_STRING);
            return 0;
        default:
            fprintf(stderr, "--help for usage\n");
            return -1;
        }

    }

    if (!strlen(procfile) || !(all|lba_a|lba_b|dump)) {
        usage(argv);
        return -1;
    }

    if ((fid=open(procfile, O_WRONLY))==-1) {
        fprintf(stderr, "error opening \"%s\" for writing (errno %i)\n", procfile, errno);
        return 1;
    }
    if (all) {
        write(fid, "reset-all", 9);
    }
    if (lba_a) {
        write(fid, "reset-lba-a", 11);
    }
    if (lba_b) {
        write(fid, "reset-lba-b", 11);
    }
    if (dump) {
        write(fid, "dump-trace", 10);
    }
    close(fid);
    return 0;
}
