/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLIG OR USING. By
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

#ifndef _UTIL_H_
#define _UTIL_H_
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/version.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#if 0
// -D_GNU_SOURCE must be set for these to be picked up
extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
extern void pthread_yield(void);
#endif

#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <signal.h>
#endif

/*
 * files
 */

#ifdef __KERNEL__ 
#define OPEN filp_open
#define SEEK(a,b,c) fh->f_op->llseek(a, b, c)
#define CLOSE(a,b) filp_close(a, b)
#define HAND struct file *
#define HAND_INIT NULL
#else
#define OPEN open
#define SEEK(a,b,c) lseek(a, b, c)
#define CLOSE(a,b) close(a)
#define HAND int
#define IS_ERR(filp) (filp==-1)
#define HAND_INIT 0
#endif

/*
 * Locks
 */
#ifdef __KERNEL__
typedef struct semaphore ISCSI_LOCK_T;
typedef struct {
    spinlock_t lock;
    unsigned long flags;
} ISCSI_SPIN_LOCK_T;
#else
typedef pthread_mutex_t ISCSI_LOCK_T;
typedef ISCSI_LOCK_T ISCSI_SPIN_LOCK_T;
#endif

#ifdef __cplusplus
#define LNKG extern "C"
#else
#define LNKG
#endif

LNKG int iscsi_lock_init(ISCSI_LOCK_T *l);
LNKG int iscsi_lock(ISCSI_LOCK_T *l);
LNKG int iscsi_lock_ni(ISCSI_LOCK_T *l);
LNKG int iscsi_unlock(ISCSI_LOCK_T *l);
LNKG int iscsi_lock_destroy(ISCSI_LOCK_T *l);

int iscsi_spin_lock_init(ISCSI_SPIN_LOCK_T *l);
int iscsi_spin_lock(ISCSI_SPIN_LOCK_T *l);
int iscsi_spin_unlock(ISCSI_SPIN_LOCK_T *l);
int iscsi_spin_lock_destroy(ISCSI_SPIN_LOCK_T *l);

#define ISCSI_LOCK_INIT_ELSE(L, ELSE)                   \
    if (iscsi_lock_init(L)!=0) {                        \
        TRACE_ERROR("iscsi_lock_init() failed\n");      \
        ELSE;                                           \
    }
#define ISCSI_LOCK_ELSE(L, ELSE)                        \
    if (iscsi_lock(L)!=0) {                             \
        if (0) TRACE_ERROR("iscsi_lock() failed\n");    \
        ELSE;                                           \
    }

#define ISCSI_LOCK_NI_ELSE(L, ELSE)                     \
    if (iscsi_lock_ni(L)!=0) {                          \
        if (0) TRACE_ERROR("iscsi_lock_ni() failed\n"); \
        ELSE;                                           \
    }

#define ISCSI_UNLOCK_ELSE(L, ELSE)              \
    if (iscsi_unlock(L)!=0) {                   \
        TRACE_ERROR("iscsi_lock() failed\n");   \
        ELSE;                                   \
    }
#define ISCSI_LOCK_DESTROY_ELSE(L, ELSE)                \
    if (iscsi_lock_destroy(L)!=0) {                     \
        TRACE_ERROR("iscsi_lock_destroy() failed\n");   \
        ELSE;                                           \
    }

#define ISCSI_SPIN_LOCK_INIT_ELSE(L, ELSE)              \
    if (iscsi_spin_lock_init(L)!=0) {                   \
        TRACE_ERROR("iscsi_spin_lock_init() failed\n"); \
        ELSE;                                           \
    }
#define ISCSI_SPIN_LOCK_ELSE(L, ELSE)                   \
    if (iscsi_spin_lock(L)!=0) {                        \
        TRACE_ERROR("iscsi_spin_lock() failed\n");      \
        ELSE;                                           \
    }
#define ISCSI_SPIN_UNLOCK_ELSE(L, ELSE)                 \
    if (iscsi_spin_unlock(L)!=0) {                      \
        TRACE_ERROR("iscsi_spin_lock() failed\n");      \
        ELSE;                                           \
    }
#define ISCSI_SPIN_LOCK_DESTROY_ELSE(L, ELSE)                   \
    if (iscsi_spin_lock_destroy(L)!=0) {                        \
        TRACE_ERROR("iscsi_spin_lock_destroy() failed\n");      \
        ELSE;                                                   \
    }

/*
 * Wait queues (condition variables)
 */

#define NO_SIGNAL (1)

#ifdef __KERNEL__
typedef wait_queue_head_t ISCSI_WQ_T;
#define ISCSI_WQ_INIT(WQ) init_waitqueue_head(WQ)
#define ISCSI_WAIT(WQ, COND) wait_event(*(WQ), COND)

static inline int iscsi_wake(int signal, ISCSI_WQ_T *wq) {
    if (signal) {
        wake_up(wq);
        return 0;
    } else {
        return 1;
    }
}

#define ISCSI_WAKE_ELSE(SIGNAL, WQ, ELSE)       \
    if (iscsi_wake(SIGNAL, WQ)!=0) {            \
        TRACE_ERROR("iscsi_wake() failed\n");   \
        ELSE;                                   \
    } 

#define ISCSI_WQ_DESTROY(WQ)
#else
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ISCSI_WQ_T;
#define ISCSI_WQ_INIT(WQ) ((pthread_mutex_init(&((WQ)->mutex), NULL)==0) \
                           &&(pthread_cond_init(&((WQ)->cond), NULL)==0))

#define ISCSI_WAIT(WQ, COND) ((pthread_mutex_lock(&((WQ)->mutex))==0) && \
                              ((COND)||                                 \
                               ((pthread_cond_wait(&((WQ)->cond),       \
                                                   &((WQ)->mutex))      \
                                 ==0))) &&                              \
                              (pthread_mutex_unlock(&((WQ)->mutex))==0))

#define ISCSI_WAKE_ELSE(SIGNAL, WQ, ELSE)  {int signal; char HACK[1];   \
        memset(HACK, 0, 1); /* stack overrun prevention HACK */         \
        if (iscsi_lock(&((WQ)->mutex))!=0) {                            \
            TRACE_ERROR("iscsi_lock() failed\n");                       \
            ELSE;                                                       \
        }                                                               \
        signal = SIGNAL;                                                \
        if (pthread_cond_signal(&((WQ)->cond))!=0) {                    \
            TRACE_ERROR("pthread_cond_signal() failed\n");              \
            ELSE;                                                       \
        }                                                               \
        if (iscsi_unlock(&((WQ)->mutex))!=0) {                          \
            TRACE_ERROR("iscsi_unlock() failed\n");                     \
            ELSE;                                                       \
        }                                                               \
        if (!signal) {                                                  \
            TRACE_ERROR("SIGNAL failed\n");                             \
            ELSE;                                                       \
        }}                                                              \
                      


#define ISCSI_WQ_DESTROY(WQ) ((pthread_cond_destroy(&((WQ)->cond))==0)  \
                              &&(pthread_mutex_destroy(&((WQ)->mutex))==0))
#endif

/*
 * Byte Order
 */

#include <byteswap.h>

#define NTOHS(x)  ntohs(x)
#define HTONS(x)  htons(x)
#define NTOHL(x)  ntohl(x)
#define HTONL(x)  htonl(x)

#ifdef __KERNEL__
#ifndef __BYTE_ORDER
#ifdef __BIG_ENDIAN
#define __BYTE_ORDER __BIG_ENDIAN
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
#define __BIG_ENDIAN 1
#endif
#endif
#ifdef __LITTLE_ENDIAN
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#endif
#endif

#ifdef __BIG_ENDIAN
#if __BYTE_ORDER == __BIG_ENDIAN
#define NTOHLL(x) (x)
#define HTONLL(x) (x)
#else
#define NTOHLL(x) bswap_64(x)
#define HTONLL(x) bswap_64(x)
#endif
#else
#define NTOHLL(x) bswap_64(x)
#define HTONLL(x) bswap_64(x)
#endif

/*
 * Time and Sleeping
 */

#ifdef __KERNEL__
#define ISCSI_SLEEP_SPIN(N) {unsigned future = jiffies+N*HZ;    \
        while (jiffies<future) ISCSI_SPIN;}
#define ISCSI_SLEEP(N) {set_current_state(TASK_INTERRUPTIBLE);  \
        schedule_timeout(N*HZ);}
#define ISCSI_USLEEP(N) {set_current_state(TASK_INTERRUPTIBLE); \
        schedule_timeout(N*HZ/1000000);}
#else
#define ISCSI_SLEEP_SPIN(T) sleep(T)
#define ISCSI_SLEEP(T) sleep(T)
#define ISCSI_USLEEP(T) usleep(T)
#define ISCSI_MSLEEP(T) usleep(T*1000)
#endif

#define NO_ACTION

#define SLEEP_UNLESS_THEN(SECS, COND, THEN) {   \
        int i;                                  \
        for (i=0; i<SECS*1000000; i+=10000) {   \
            if (COND) {THEN; break;}            \
            ISCSI_USLEEP(10000);                \
        }                                       \
    }

#define toSeconds(t) (t.tv_sec+t.tv_usec/1e6)


/*
 * Kernel Versioning
 */

#ifdef __KERNEL__
#if !defined(LINUX_VERSION_CODE)
#include <linux/version.h>
#endif
#define LinuxVersionCode(v, p, s) (((v)<<16)+((p)<<8)+(s))
#endif

/* 
 * Memory allocation
 */

void *iscsi_malloc(uint64_t n);
void iscsi_free(void *ptr);
void *iscsi_malloc_atomic(int n);
void iscsi_free_atomic(void *ptr);
int iscsi_malloc_atomic_init(void);
void iscsi_malloc_atomic_shutdown(void);

/*
 * Comparison
 */

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MIN_3(A,B,C) (((A)<(B))?(((A)<(C))?(A):(C)):(((B)<(C))?(B):(C)))

/* 
 * Tags
 */
 
#define ISCSI_SET_TAG_ELSE(tag, tid, ELSE)              \
    {                                                   \
        ISCSI_SPIN_LOCK_ELSE(&g_tag_lock[tid], ELSE);   \
        *tag = g_tag++;                                 \
        ISCSI_SPIN_UNLOCK_ELSE(&g_tag_lock[tid], ELSE); \
    }

/* 
 * Hashing
 */

typedef struct hash_elem_t {
    struct hash_elem_t *next;
    unsigned key;
    void *ptr;
} HASH_ELEM_T;

typedef struct hash_t {
    void **bucket;
    int collisions;
    int insertions;
    int n;
    ISCSI_SPIN_LOCK_T lock;
} HASH_T;

int hash_init(HASH_T *h, int n);
int hash_insert(HASH_T *h, HASH_ELEM_T *elem);
void *hash_remove(HASH_T *h, unsigned tag);
HASH_ELEM_T *hash_remove_elem(HASH_T *h, unsigned tag);
void *hash_remove_head(HASH_T *h);
void *hash_get(HASH_T *h, unsigned tag);
HASH_ELEM_T *hash_get_elem(HASH_T *h, unsigned tag);
int hash_count(HASH_T *h);
int hash_destroy(HASH_T *h);
int hash_print(HASH_T *h);

#define HASH_INSERT_ELSE(HASH, PTR, TAG, ELSE)                  \
    TRACE(TRACE_HASH, 0, "hashing 0x%x (ptr %p)\n", TAG, PTR);  \
    PTR->hash.key = TAG;                                        \
    PTR->hash.ptr = PTR;                                        \
    if (hash_insert(HASH, &PTR->hash)!=0) {                     \
        TRACE_ERROR("hash_insert() failed\n");                  \
        ELSE;                                                   \
    }

#define HASH_REMOVE_ELSE(HASH, TAG, ELSE)               \
    TRACE(TRACE_HASH, 0, "removing 0x%x\n", TAG);       \
    if (hash_remove(HASH, TAG)==NULL) {                 \
        TRACE_ERROR("hash_remove(0x%x) failed\n", TAG); \
        ELSE;                                           \
    }

#define HASH_GET_ELSE(HASH, TAG, PTR, ELSE)             \
    TRACE(TRACE_HASH, 0, "getting 0x%x\n", TAG);        \
    if ((PTR = hash_get(HASH, TAG))==NULL) {            \
        TRACE_ERROR("hash_get() failed\n");             \
        ELSE;                                           \
    }


/*
 * Queuing
 */

typedef struct iscsi_queue_t {
    int head, tail, count;
    void **elem;
    int depth;
    ISCSI_SPIN_LOCK_T lock;
} ISCSI_QUEUE_T;

int iscsi_queue_init(ISCSI_QUEUE_T *q, int depth);
void iscsi_queue_destroy(ISCSI_QUEUE_T *q);
int iscsi_queue_insert(ISCSI_QUEUE_T *q, void *elem);
void *iscsi_queue_remove(ISCSI_QUEUE_T *q);
void *iscsi_queue_remove_this(ISCSI_QUEUE_T *q, void *ptr);
void *iscsi_queue_get_this(ISCSI_QUEUE_T *q, void *ptr);
void *iscsi_queue_get_head(ISCSI_QUEUE_T *q);
int iscsi_queue_depth(ISCSI_QUEUE_T *q);
int iscsi_queue_full(ISCSI_QUEUE_T *q);
int iscsi_queue_print(ISCSI_QUEUE_T *q);

#define QUEUE_INIT_ELSE(Q, DEPTH, ELSE)                 \
    if (iscsi_queue_init(Q, DEPTH)!=0) {                \
        TRACE_ERROR("iscsi_queue_init() failed\n");     \
        ELSE;                                           \
    }

#define QUEUE_INSERT_ELSE(Q, ELEM, ELSE)                \
    if (iscsi_queue_insert(Q, ELEM)!=0) {               \
        TRACE_ERROR("iscsi_queue_insert() failed\n");   \
        ELSE;                                           \
    }

#define QUEUE_REMOVE_ELSE(Q, ELEM, ELSE)                \
    if ((ELEM=iscsi_queue_remove(Q))==NULL) {           \
        TRACE_ERROR("iscsi_queue_remove() failed\n");   \
        ELSE;                                           \
    }

#define QUEUE_REMOVE_THIS_ELSE(Q, THIS, ELEM, ELSE)             \
    if ((ELEM=iscsi_queue_remove_this(Q, THIS))==NULL) {        \
        TRACE_ERROR("iscsi_queue_remove_this() failed\n");      \
        ELSE;                                                   \
    }

/*
 * Socket abstraction
 */

#ifdef __KERNEL__
typedef struct socket* iscsi_socket_t;
#else
typedef int iscsi_socket_t;
#endif

// Turning off Nagle's Algorithm doesn't always seem to work,
// so we combine two messages into one when the second's size
// is less than or equal to ISCSI_SOCK_HACK_CROSSOVER.

#define ISCSI_SOCK_HACK_CROSSOVER    0
#define ISCSI_SOCK_CONNECT_NONBLOCK  0
#define ISCSI_SOCK_CONNECT_TIMEOUT   1
#define ISCSI_SOCK_MSG_BYTE_ALIGN    4

/*
 * Socket operations
 */

#define ISCSI_SOCK_SHUTDOWN_RECV 0
#define ISCSI_SOCK_SHUTDOWN_SEND 1

int iscsi_sock_create(iscsi_socket_t *sock, int family);
int iscsi_sock_setsockopt(iscsi_socket_t *sock, int level, int optname, 
                          void *optval, unsigned optlen);
int iscsi_sock_getsockopt(iscsi_socket_t *sock, int level, int optname,
                          void *optval, unsigned *optlen);
int iscsi_sock_bind(iscsi_socket_t sock, int port, int family);
int iscsi_sock_listen(iscsi_socket_t sock);
int iscsi_sock_connect(iscsi_socket_t sock, char *hostname, int port, 
                       int family);
int iscsi_sock_accept(iscsi_socket_t sock, iscsi_socket_t *newsock);
int iscsi_sock_shutdown(iscsi_socket_t sock, int how);
int iscsi_sock_close(iscsi_socket_t sock);
int iscsi_sock_msg(iscsi_socket_t sock, int send, int len, void *data, 
                   int sg_len);
int iscsi_sock_send_header_and_data(iscsi_socket_t sock,
                                    void *header, int header_len,
                                    void *data, int data_len, int iov_len);
int iscsi_sock_getsockname(iscsi_socket_t sock, struct sockaddr *name, 
                           unsigned *namelen);
int iscsi_sock_getpeername(iscsi_socket_t sock, struct sockaddr *name, 
                           unsigned *namelen);
int modify_iov(struct iovec **iov, int *iov_len, unsigned offset, 
               unsigned length);

/*
 * Threading routines
 */

typedef struct iscsi_thread_t {
#ifdef __KERNEL__
    struct task_struct *pthread;
#else
    pthread_t pthread;
#endif
} ISCSI_THREAD_T;

int iscsi_thread_create(ISCSI_THREAD_T *thread, void * (*proc)(void *), 
                        void *arg);
int iscsi_thread_create_joinable(ISCSI_THREAD_T *thread, 
                                 void * (*proc)(void *), void *arg);

#ifdef __KERNEL__
#define ISCSI_SET_THREAD(ME) me->thread.pthread = current;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define DAEMONIZE_ARG(NAME) NAME
#define REPARENT_TO_INIT()
#else
#define DAEMONIZE_ARG(NAME)
#define REPARENT_TO_INIT() reparent_to_init()
#endif
#define ISCSI_THREAD_START(NAME)                \
    lock_kernel();                              \
    daemonize(DAEMONIZE_ARG(NAME));             \
    REPARENT_TO_INIT();                         \
    unlock_kernel();                            \
    sprintf(current->comm, "%s", NAME);
#else
#define ISCSI_SET_THREAD(ME)  /* set by pthread_create in iscsi_thread_create */
#define ISCSI_THREAD_START(NAME)
#endif


/*
 * Worker utilities
 */

#define WORKER_STATE_STARTED   0x00000001
#define WORKER_STATE_ERROR     0x00000002
#define WORKER_STATE_STOPPING  0x00000004
#define WORKER_STATE_STOPPED   0x00000008
#define WORKER_STATE_WORK      0x00000010

#define WORKER_STARTED(W) (W.state & WORKER_STATE_STARTED)
#define WORKER_ERROR(W) (W.state & WORKER_STATE_ERROR)
#define WORKER_STOPPING(W) (W->state & WORKER_STATE_STOPPING)
#define WORKER_STOPPED(W) (W.state & WORKER_STATE_STOPPED)
#define SET_WORK(W) ((W)->state |= WORKER_STATE_WORK);
#define HAS_WORK(W) (((W)->state & WORKER_STATE_WORK)!=0)
#define CLEAR_WORK(W) ((W)->state &= ~WORKER_STATE_WORK);

typedef struct {
    char name[256];
    int id;
    ISCSI_THREAD_T thread;
    volatile unsigned state;
    ISCSI_WQ_T wq;
    void *ptr;
} ISCSI_WORKER_T;

/*
 * A worker calls these when starting.  If the condition is met, then it
 * starts successfully, otherwise it immediately exits. Note: we'll need
 * to add an ELSE clause to this in case there's any cleanup.
 */

#ifndef __KERNEL__
#define START_ELSE_EXIT(ME,COND) {                                      \
        TRACE(TRACE_WORKER, 1, "\"%s\" starting\n", ME->name);          \
        ISCSI_THREAD_START(ME->name);                                   \
        {                                                               \
            sigset_t sigset;                                            \
            sigemptyset(&sigset);                                       \
            sigaddset(&sigset, SIGINT);                                 \
        }                                                               \
        ISCSI_SET_THREAD(ME);                                           \
        if (!COND) {                                                    \
            TRACE_WORKER(0, "start failed for \"%s\"\n", ME->name);     \
            ME->state |= WORKER_STATE_STARTED;                          \
            EXIT(me, -1);                                               \
        }                                                               \
        TRACE(TRACE_WORKER, 1, "\"%s\" started\n", ME->name);           \
        ISCSI_WAKE_ELSE(ME->state|=WORKER_STATE_STARTED, &ME->wq,       \
                        EXIT(me, -1));                                  \
    }                                                 
#else
#define START_ELSE_EXIT(ME,COND) {                                            \
        TRACE(TRACE_WORKER, 1, "\"%s\" starting\n", ME->name);                \
        ISCSI_THREAD_START(ME->name);                                         \
        ISCSI_SET_THREAD(ME);                                                 \
        if (!COND) {                                                          \
            TRACE(TRACE_WORKER, 0, "start failed for \"%s\"\n", ME->name);    \
            ME->state |= WORKER_STATE_STARTED;                                \
            EXIT(me, -1);                                                     \
        }                                                                     \
        ISCSI_WAKE_ELSE(ME->state|=WORKER_STATE_STARTED, &ME->wq,             \
                        EXIT(me, -1);                                         \
                        TRACE(TRACE_WORKER, 1, "\"%s\" started\n", ME->name); \
                        }
#endif

#ifndef __KERNEL__
#define START_ELSE(ME,COND,ELSE) {                                           \
        TRACE(TRACE_WORKER, 1, "\"%s\" starting\n", ME->name);               \
        ISCSI_THREAD_START(ME->name);                                        \
        {                                                                    \
            sigset_t sigset;                                                 \
            sigemptyset(&sigset);                                            \
            sigaddset(&sigset, SIGINT);                                      \
        }                                                                    \
        ISCSI_SET_THREAD(ME);                                                \
        if (!COND) {                                                         \
            TRACE(TRACE_WORKER, 0, "start failed for \"%s\"\n", ME->name);   \
            ME->state |= WORKER_STATE_STARTED;                               \
            ELSE;                                                            \
        } else {                                                             \
            TRACE(TRACE_WORKER, 1, "\"%s\" started\n", ME->name);            \
            ISCSI_WAKE_ELSE(ME->state|=WORKER_STATE_STARTED, &ME->wq, ELSE); \
        }                                                                    \
    }                                                 
#else
#define START_ELSE(ME,COND,ELSE) {                                           \
        TRACE(TRACE_WORKER, 1, "\"%s\" starting\n", ME->name);               \
        ISCSI_THREAD_START(ME->name);                                        \
        ISCSI_SET_THREAD(ME);                                                \
        if (!COND) {                                                         \
            TRACE(TRACE_WORKER, 0, "start failed for \"%s\"\n", ME->name);   \
            ME->state |= WORKER_STATE_STARTED;                               \
            ELSE;                                                            \
        } else {                                                             \
            TRACE(TRACE_WORKER, 1, "\"%s\" started\n", ME->name);            \
            ISCSI_WAKE_ELSE(ME->state|=WORKER_STATE_STARTED, &ME->wq, ELSE); \
        }                                                                    \
    }
#endif

#define EXIT(ME, RC)                                                         \
    TRACE(TRACE_WORKER, 1, "\"%s\" exiting (rc %i)\n", ME->name, RC)         \
    if (RC!=0) ME->state |= WORKER_STATE_ERROR;                              \
    ISCSI_WAKE_ELSE(ME->state|=WORKER_STATE_STOPPED, &(ME->wq), return RC);  \
    return RC;

/*
 * These are called by the thread managing the worker.
 */

#define START_WORKER_ELSE(NAME, ID, W, PROC, PTR, ELSE) {                     \
        TRACE(TRACE_WORKER, 0, "starting \"%s[%"PRIu64"]\"\n", NAME,          \
              (uint64_t) ID);                                                 \
        memset(&W, 0, sizeof(ISCSI_WORKER_T));                                \
        ISCSI_WQ_INIT(&(W.wq));                                               \
        W.ptr = PTR;                                                          \
        W.id = ID;                                                            \
        sprintf(W.name, "%s[%"PRIu64"]", NAME, (uint64_t) ID);                \
        TRACE(TRACE_WORKER, 1, "initialized \"%s %"PRIu64"\"\n", NAME,        \
              (uint64_t) ID);                                                 \
        if (iscsi_thread_create_joinable(&W.thread, (void *) &PROC, &W)!=0) { \
            TRACE_ERROR("iscsi_thread_create_joinable() failed\n");           \
            ELSE;                                                             \
        }                                                                     \
        TRACE(TRACE_WORKER, 1, "created thread, waiting for \"%s\"\n",        \
              W.name);                                                        \
        ISCSI_WAIT(&(W.wq), WORKER_STARTED(W));                               \
        if (WORKER_ERROR(W))      {                                           \
            TRACE(TRACE_WORKER, 0, "failed to start \"%s\"\n",  W.name);      \
            ELSE;                                                             \
        } else {                                                              \
            TRACE(TRACE_WORKER, 0, "started \"%s\" ok\n", W.name);            \
        }                                                                     \
    }

#define START_WORKER_NO_JOIN_ELSE(NAME, ID, W, PROC, PTR, ELSE) {             \
        TRACE(TRACE_WORKER, 0, "starting \"%s[%"PRIu64"]\"\n", NAME,          \
              (uint64_t) ID);                                                 \
        memset(&W, 0, sizeof(ISCSI_WORKER_T));                                \
        ISCSI_WQ_INIT(&(W.wq));                                               \
        W.ptr = PTR;                                                          \
        W.id = ID;                                                            \
        sprintf(W.name, "%s[%"PRIu64"]", NAME, (uint64_t) ID);                \
        TRACE(TRACE_WORKER, 1, "initialized \"%s %"PRIu64"\"\n", NAME,        \
              (uint64_t) ID);                                                 \
        if (iscsi_thread_create(&W.thread, (void *) &PROC, &W)!=0) {          \
            TRACE_ERROR("iscsi_thread_create() failed\n");                    \
            ELSE;                                                             \
        }                                                                     \
        TRACE(TRACE_WORKER, 1, "created thread, waiting for \"%s\"\n",        \
              W.name);                                                        \
        ISCSI_WAIT(&(W.wq), WORKER_STARTED(W));                               \
        if (WORKER_ERROR(W))      {                                           \
            TRACE(TRACE_WORKER, 0, "failed to start \"%s\"\n",  W.name);      \
            ELSE;                                                             \
        } else {                                                              \
            TRACE(TRACE_WORKER, 0, "started \"%s\" ok\n", W.name);            \
        }                                                                     \
    }

#define NO_SIGNAL (1)
#ifdef __KERNEL__
#define STOP_WORKER_ELSE(W, SIGNAL, ELSE) {                             \
        TRACE(TRACE_WORKER, 0, "stopping \"%s\"\n", W.name);            \
        ISCSI_WAKE_ELSE(((W.state|=WORKER_STATE_STOPPING)&&(SIGNAL)),   \
                        (&(W.wq)),                                      \
                        ELSE);                                          \
        ISCSI_WAIT(&W.wq, WORKER_STOPPED(W));                           \
        TRACE(TRACE_WORKER, 0, "\"%s\" stopped\n", W.name);             \
        ISCSI_WQ_DESTROY(&W.wq);                                        \
    }
#else
#define STOP_WORKER_ELSE(W, SIGNAL, ELSE) {                             \
        TRACE(TRACE_WORKER, 0, "stopping \"%s\"\n", W.name);            \
        ISCSI_WAKE_ELSE(((W.state|=WORKER_STATE_STOPPING)&&(SIGNAL)),   \
                        (&(W.wq)),                                      \
                        ELSE);                                          \
        ISCSI_WAIT(&W.wq, WORKER_STOPPED(W));                           \
        {int rc; pthread_join(W.thread.pthread, (void *) &rc);}         \
        TRACE(TRACE_WORKER, 0, "\"%s\" stopped\n", W.name);             \
        ISCSI_WQ_DESTROY(&W.wq);                                        \
    }
#endif

/*
 * Spin locks
 */

#ifdef __KERNEL__
#define ISCSI_SPIN schedule()
#else
#define ISCSI_SPIN sched_yield()
#endif

/*
 * Pre/Post condition checking 
 */

#define NO_CLEANUP {}
#define RETURN_GREATER(NAME, V1, V2, CU, RC)                    \
    if ((V1)>(V2)) {                                            \
        TRACE_ERROR("Bad \"%s\": %u > %u.\n", NAME, V1, V2);    \
        CU;                                                     \
        return RC;                                              \
    }

#define WARN_GREATER(NAME, V1, V2)                              \
    if ((V1)>(V2)) {                                            \
        TRACE_WARN("Bad \"%s\": %u > %u.\n", NAME, V1, V2);     \
    }

#define RETURN_NOT_EQUAL(NAME, V1, V2, CU, RC)                          \
    if ((V1)!=(V2)) {                                                   \
        TRACE_ERROR("Bad \"%s\": Got %u expected %u.\n", NAME, V1, V2); \
        CU;                                                             \
        return RC;                                                      \
    }

#define NOT_NULL_ELSE(NAME, V1, ELSE)                           \
    if ((V1)==NULL) {                                           \
        TRACE_ERROR("Bad pointer\"%s\": NULL value\n", NAME);   \
        ELSE;                                                   \
    }

#define NOT_EQUAL_ELSE(NAME, V1, V2, ELSE)                                 \
    if ((uint64_t)(V1)==(uint64_t)(V2)) {                                  \
        TRACE_ERROR("Bad \"%s\": Got %"PRIu64" expected not %"PRIu64".\n", \
                    NAME, (uint64_t) V1, (uint64_t) V2);                   \
        ELSE;                                                              \
    }

#define EQUAL_ELSE(NAME, V1, V2, ELSE)                                       \
    if ((uint64_t)(V1)!=(uint64_t)(V2)) {                                    \
        TRACE_ERROR("Bad \"%s\": Got %"PRIu64" expected %"PRIu64".\n", NAME, \
                    (uint64_t) V1, (uint64_t) V2);                           \
        ELSE;                                                                \
    }

#define GREATER_THAN_ELSE(NAME, V1, V2, ELSE)                             \
    if ((V1)<=(V2)) {                                                     \
        TRACE_ERROR("Bad \"%s\": Got %u expected > %u.\n", NAME, V1, V2); \
        ELSE;                                                             \
    }

#define LESS_THAN_ELSE(NAME, V1, V2, ELSE)                                \
    if ((V1)>=(V2)) {                                                     \
        TRACE_ERROR("Bad \"%s\": Got %"PRIu64" expected < %"PRIu64".\n",  \
                    NAME, (uint64_t) V1, (uint64_t) V2);                  \
        ELSE;                                                             \
    }

#define LESS_THAN_EQUAL_ELSE(NAME, V1, V2, ELSE)                           \
    if ((V1)>(V2)) {                                                       \
        TRACE_ERROR("Bad \"%s\": Got %u expected <= %u.\n", NAME, V1, V2); \
        ELSE;                                                              \
    }

#define WARN_NOT_EQUAL(NAME, V1, V2)                                    \
    if ((V1)!=(V2)) {                                                   \
        TRACE_WARN("Bad \"%s\": Got %u expected %u.\n", NAME, V1, V2);  \
    }

#define RETURN_EQUAL(NAME, V1, V2, CU, RC)                      \
    if ((V1)==(V2)) {                                           \
        TRACE_ERROR("Bad \"%s\": %u == %u.\n", NAME, V1, V2);   \
        CU;                                                     \
        return RC;                                              \
    }

/*
 * Misc
 */

unsigned iscsi_atoi(char *value);
int HexTextToData(const char *text, unsigned int textLength,
                  unsigned char *data, unsigned int DataLength);

int HexDataToText(unsigned char *data, unsigned int dataLength,
                  char *text, unsigned int textLength);

void GenRandomData(unsigned char *data, unsigned int length);
int command_pipe(char *cmd, char *buffer, int len);
#endif /* _UTIL_H_ */
