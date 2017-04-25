
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __KERNEL__
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#endif
#include "config.h"
#include "ost.h"

#define TRACE_DEBUG      0x00000001
#define TRACE_MEM        0x00000002
#define TRACE_HASH       0x00000004
#define TRACE_NET        0x00000008
#define TRACE_IOV        0x00000010
#define TRACE_PARAM      0x00000020
#define TRACE_RPC        0x00000040
#define TRACE_SYNC       0x00000080
#define TRACE_SCSI       0x00000100
#define TRACE_ISCSI      0x00000200
#define TRACE_OSDFS      0x00000400
#define TRACE_SO         0x00000800
#define TRACE_SESSION    0x00001000
#define TRACE_WORKER     0x00002000
#define TRACE_SECURITY   0x00004000
#define TRACE_EH         0x00008000
#define TRACE_ALL        0xffffffff

#define ISCSI_DEBUG_LEVEL  0
#define ISCSI_DEBUG_ENABLE TRACE_SCSI
int iscsi_debug_init(void);
int iscsi_debug_shutdown(void);
int iscsi_debug_lock(void);
int iscsi_debug_unlock(void);

#ifdef __KERNEL__
#define DEBUG_INIT
#define DEBUG_SHUTDOWN
#else
#define DEBUG_INIT iscsi_debug_init()
#define DEBUG_SHUTDOWN  iscsi_debug_shutdown()
#endif

extern char g_buff[8192];

/*
 * printf() and printk()
 */

#ifdef __KERNEL__
#define PRINT printk
#else
#define PRINT printf
#endif

#ifdef CONFIG_ISCSI_DEBUG

#define TRACE(type, level, args...)                                        \
    if ((level <= ISCSI_DEBUG_LEVEL) && (type & (ISCSI_DEBUG_ENABLE))) {   \
        iscsi_debug_lock();                                                \
        sprintf(g_buff, args);                                             \
        PRINT("pid %i: %s:%s:%d: %s", OST_GETPID, __FILE__, __FUNCTION__,\
              __LINE__, g_buff);                                           \
        iscsi_debug_unlock();                                              \
    }

#define TRACE_CLEAN(type, level, args...)                                \
    if ((level <= ISCSI_DEBUG_LEVEL) && (type & (ISCSI_DEBUG_ENABLE))) { \
        iscsi_debug_lock();                                              \
        sprintf(g_buff, args);                                           \
        PRINT("%s", g_buff);                                             \
        iscsi_debug_unlock();                                            \
    }

#define PRINT_BUFF(type, level, buff, len, width)                        \
    if ((level <= ISCSI_DEBUG_LEVEL) && (type & (ISCSI_DEBUG_ENABLE))) { \
        int trace_i;                                                     \
                                                                         \
        iscsi_debug_lock();                                              \
        for (trace_i=0; trace_i<len; trace_i++) {                        \
            if (trace_i%width==0) {                                      \
                if (trace_i) PRINT("\n");                                \
                PRINT("%6i: ", trace_i);                                 \
            }                                                            \
            PRINT("%2x ", (unsigned char) (buff)[trace_i]);              \
        }                                                                \
        if ((len+1)%width) PRINT("\n");                                  \
        iscsi_debug_unlock();                                            \
    }

#define PRINT_BUFF_OFF(type, level, buff, len, width, offset)            \
    if ((level <= ISCSI_DEBUG_LEVEL) && (type & (ISCSI_DEBUG_ENABLE))) { \
        int trace_i;                                                     \
                                                                         \
        iscsi_debug_lock();                                              \
        for (trace_i=0; trace_i<len; trace_i++) {                        \
            if (trace_i%width==0) {                                      \
                if (trace_i) PRINT("\n");                                \
                PRINT("%6i: ", trace_i+offset);                          \
            }                                                            \
            PRINT("%2x ", (unsigned char) (buff)[trace_i]);              \
        }                                                                \
        if ((len+1)%width) PRINT("\n");                                  \
        iscsi_debug_unlock();                                            \
    }

#else

#define DEBUG_INIT
#define DEBUG_SHUTDOWN
#define TRACE(type, level, args...)
#define TRACE_CLEAN(type, level, args...)
#define PRINT_BUFF(type, level, buff, size, width)

#endif

#define TRACE_WARN(args...) {                                   \
        iscsi_debug_lock();                                     \
        sprintf(g_buff, args);                                  \
        PRINT("%s:%s:%i: WARNING: %s",                          \
              __FILE__, __FUNCTION__, __LINE__, g_buff);        \
        iscsi_debug_unlock();                                   \
    }

#ifdef __KERNEL__
#define TRACE_ERROR(args...) {                                          \
        iscsi_debug_lock();                                             \
        sprintf(g_buff, args);                                          \
        PRINT("%s:%s:%i:pid %i: *** ERROR *** %s",                      \
              __FILE__, __FUNCTION__, __LINE__, OST_GETPID, g_buff);    \
        iscsi_debug_unlock();                                           \
    }
#else
#define TRACE_ERROR(args...) {                                                      \
        iscsi_debug_lock();                                                         \
        sprintf(g_buff, args);                                                      \
        fprintf(stderr, "%s:%s:%i:pid %i: tid %li: *** ERROR *** %s",               \
                __FILE__, __FUNCTION__, __LINE__, OST_GETPID, OST_GETTID, g_buff);  \
        iscsi_debug_unlock();                                                       \
    }
#endif

#ifndef __KERNEL__
#define PANIC(args...) {                                                \
        TRACE_ERROR("---BEGIN PANIC---\n");                             \
	system("date 1>&2");                                            \
        TRACE_ERROR(args);                                              \
        TRACE_ERROR("----END PANIC----\n");                             \
	exit(1);                                                        \
        ISCSI_SLEEP(3600);                                              \
    }
#else
#define PANIC(args...) {                                                \
        TRACE_ERROR("---BEGIN PANIC---\n");                             \
        TRACE_ERROR(args);                                              \
        TRACE_ERROR("----END PANIC----\n");                             \
        ISCSI_SLEEP(3600);                                              \
    }
#endif

#ifdef __KERNEL__
#define ASSERT(expr)
#else
#define ASSERT(expr) {					          \
	if ((expr) ==  0) { 				          \
	    TRACE_ERROR("Assertion failed: %s\n",__STRING(expr)); \
	    exit(1);						  \
	}                                                         \
    }
#endif

#endif /* _DEBUG_H_ */
