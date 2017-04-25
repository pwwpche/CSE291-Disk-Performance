
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

#ifdef __KERNEL__
#include <asm/uaccess.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/string.h>
#include <asm/hardirq.h>
#include <linux/vmalloc.h>
#include <linux/random.h>
#define PRIu64 "llu"
#else
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/uio.h>
#include <pthread.h>
#include <errno.h>
#include <inttypes.h>
#endif
#include "util.h"
#include "debug.h"
#include "initiator.h"
#include "ost.h"

/*
 * Memory Allocation
 */

#ifdef __KERNEL__

#undef ISCSI_MALLOC_POOL

#ifdef ISCSI_MALLOC_POOL
#define ATOMIC_SIZE 131072
#define ATOMIC_NUM  256

static mempool_t *g_atomic_data_pool = NULL;
static struct pool_info *g_atomic_data_pool_info = NULL;

static void *atomic_data_pool_alloc(gfp_t gfp_flags, void *pool_info) {
    return kzalloc(ATOMIC_SIZE, gfp_flags);
}

static void atomic_data_pool_free(void *data, void *pool_info) {
    kfree(data);
}

int iscsi_malloc_atomic_init(void) {
    PRINT("iscsi_malloc_atomic_init()\n");
    if ((g_atomic_data_pool = mempool_create(ATOMIC_NUM,
                                             atomic_data_pool_alloc,
                                             atomic_data_pool_free,
                                             g_atomic_data_pool_info))==NULL) {
        TRACE_ERROR("mempool_create() failed\n");
        return -1;
    }

    return 0;
}

void iscsi_malloc_atomic_shutdown(void) {
    PRINT("iscsi_malloc_atomic_shutdown()\n");
    mempool_destroy(g_atomic_data_pool);
    return;
}

void *iscsi_malloc_atomic(int n) {
    void *ptr;

    if (n > ATOMIC_SIZE) {
        TRACE_ERROR("exceeded maximum allocation size (%i)\n", n);
        return NULL;
    }
    if ((ptr = mempool_alloc(g_atomic_data_pool, GFP_KERNEL))==NULL) {
        TRACE_ERROR("mempool_alloc() failed\n");
        return NULL;
    }
    TRACE(TRACE_MEM, 0, "iscsi_malloc_atomic(%i) = 0x%p\n", n, ptr);

    return ptr;
}
void iscsi_free_atomic(void *ptr) {
    mempool_free(ptr, g_atomic_data_pool);
    TRACE(TRACE_MEM, 0, "iscsi_free_atomic(0x%p)\n", ptr);
}
#else
int iscsi_malloc_atomic_init(void) {
    //PRINT("iscsi_malloc_atomic_init() -- NOP\n");

    return 0;
}

void iscsi_malloc_atomic_shutdown(void) {
    //PRINT("iscsi_malloc_atomic_shutdown() -- NOP\n");

    return;
}

void *iscsi_malloc_atomic(int n) {
    TRACE(TRACE_MEM, 0, "iscsi_malloc_atomic(%i)\n", n);

    return kmalloc(n, GFP_ATOMIC);
}

void iscsi_free_atomic(void *ptr) {
    TRACE(TRACE_MEM, 0, "iscsi_free_atomic(0x%p)\n", ptr);
    kfree(ptr);
}
#endif
#else
void *iscsi_malloc_atomic(int n) {
    void *ptr;

    ptr = malloc(n);
    TRACE(TRACE_MEM, 0, "iscsi_malloc_atomic(%i) = 0x%p\n", n, ptr);
    return ptr;
}
void iscsi_free_atomic(void *ptr) {
    free(ptr);
    TRACE(TRACE_MEM, 0, "iscsi_free_atomic(0x%p)\n", ptr);
}
#endif

#if 1
void *iscsi_malloc(uint64_t n) {
    void *ptr;

#ifdef __KERNEL__
    ptr = vmalloc(n);
#else
    ptr = malloc(n);
#endif
    TRACE(TRACE_MEM, 0, "iscsi_malloc_atomic(%"PRIu64") = 0x%p\n", n, ptr);
    return ptr;
}

void iscsi_free(void *ptr) {
#ifdef __KERNEL__
    vfree(ptr);
#else
    free(ptr);
#endif
    TRACE(TRACE_MEM, 0, "iscsi_free(0x%p)\n", ptr);
}
#else
void *iscsi_malloc(uint64_t n) {
    return iscsi_malloc_atomic(n);
}
void iscsi_free(void *ptr) {
    return iscsi_free_atomic(ptr);
}
#endif

/*
 * Threading Routines
 */

int *test_proc(void * arg) {
    return 0;
}

int iscsi_thread_create_joinable(ISCSI_THREAD_T *thread, 
                                 void * (*proc)(void *), void *arg) {
#ifdef __KERNEL__
    kernel_thread((int (*)(void *)) proc, arg, 0);
#else
    int retry = 1;
again:
    if (pthread_create(&thread->pthread, NULL, proc, arg)!=0) {
        TRACE_ERROR("pthread_create() failed (errno %i)\n", errno);
	if (retry) {
	    sleep(1);
            goto again;
        }
        return -1;
    }
#endif
    return 0;
}

int iscsi_thread_create(ISCSI_THREAD_T *thread, void * (*proc)(void *), 
                        void *arg) {
#ifdef __KERNEL__
    kernel_thread((int (*)(void *)) proc, arg, 0);
#else
    if (pthread_create(&thread->pthread, NULL, proc, arg)!=0) {
        TRACE_ERROR("pthread_create() failed\n");
        return -1;
    } 
    if (pthread_detach(thread->pthread)!=0) {
        TRACE_ERROR("pthread_detach() failed\n");
        return -1;
    }
#endif
    return 0;
}

/*
 * Queuing Functions
 */

int iscsi_queue_init(ISCSI_QUEUE_T *q, int depth) {
    q->head = q->tail = q->count = 0; q->depth = depth;
    if ((q->elem=iscsi_malloc_atomic(depth*sizeof(void *)))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }
    ISCSI_SPIN_LOCK_INIT_ELSE(&q->lock, return -1);
    return 0;
}

void iscsi_queue_destroy(ISCSI_QUEUE_T *q) {
    /*WARN_NOT_EQUAL("q->count", q->count, 0);*/
    iscsi_free_atomic(q->elem);
}

int iscsi_queue_full(ISCSI_QUEUE_T *q) {
    return (q->count == q->depth);
}

int iscsi_queue_depth(ISCSI_QUEUE_T *q) {
    return q->count;
}

int iscsi_queue_insert(ISCSI_QUEUE_T *q, void *ptr) {
    ISCSI_SPIN_LOCK_ELSE(&q->lock, return -1);
    if (iscsi_queue_full(q)) {
        TRACE_ERROR("QUEUE FULL at %i\n", q->count);
        ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return -1);
        return -1;
    }
    q->elem[q->tail] = ptr;
    q->tail++;
    if (q->tail == q->depth) {
        q->tail = 0;
    }
    q->count++;
    ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return -1);

    return 0;
}

void *iscsi_queue_remove(ISCSI_QUEUE_T *q) {
    void *ptr;

    ISCSI_SPIN_LOCK_ELSE(&q->lock, return NULL);
    if (!iscsi_queue_depth(q)) {
        ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
        return NULL;
    }
    q->count--;
    ptr = q->elem[q->head];
    q->head++;
    if (q->head == q->depth) {
        q->head = 0;
    }
    ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);

    return ptr;
}

int iscsi_queue_print(ISCSI_QUEUE_T *q) {
    int i;

    ISCSI_SPIN_LOCK_ELSE(&q->lock, return -1);
    if (!q->count) {
        PRINT("queue empty\n");
        ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return -1);
        return 0;
    }
    for (i=q->head;; i++) {
        if (i == q->depth) i=0;
        if (i == q->tail) break;
        PRINT("elem[%i] = %p (0x%x)\n", i, q->elem[i], 
              ((INITIATOR_CMD_T*)q->elem[i])->tag);
    }
    ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return -1);
    return 0;
}

void *iscsi_queue_get_head(ISCSI_QUEUE_T *q) {
    return q->elem[q->head];
}

void *iscsi_queue_get_this(ISCSI_QUEUE_T *q, void *this) {
    int pos;

    if (!iscsi_queue_depth(q)) return NULL;
    ISCSI_SPIN_LOCK_ELSE(&q->lock, return NULL);
    for (pos=q->head;;pos++) {
        if (pos == q->depth) pos = 0;    
        if (q->elem[pos] == this) {
            ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
            return q->elem[pos];
        }
        if (pos == q->tail) {
            ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
            return NULL;
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
    return NULL;
}

/* buggy code */
void *iscsi_queue_remove_this(ISCSI_QUEUE_T *q, void *this) {
    int pos;
    int i;

    if (!iscsi_queue_depth(q)) return NULL;
    ISCSI_SPIN_LOCK_ELSE(&q->lock, return NULL);
    for (pos = q->head;;pos++) {
        if (pos == q->depth) pos = 0;    
        if (q->elem[pos] == this) {
            q->count--;
            if (q->head == pos) {
                q->head++;
            } else if (q->head < q->tail) {
                for (i=pos; i>q->head; i--) {
                    q->elem[i] = q->elem[i-1];              
                } 
                q->head++;
            } else if (q->head == q->tail) {            
                /* do nothing */
            } else {
                if (pos<q->head) {
                    for (i=pos; i>0; i--) {
                        q->elem[i] = q->elem[i-1];                  
                    }
                    q->elem[0] = q->elem[q->depth-1];
                    for (i=q->depth-1; i>q->head; i--) {
                        q->elem[i] = q->elem[i-1];                  
                    }
                } else {
                    for (i=pos; i>q->head; i--) {
                        q->elem[i] = q->elem[i-1];                  
                    }
                }
                q->head++;
            }
            if (q->head == q->depth) q->head = 0;
            ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
            return this;
        }
        if (pos == q->tail) {
            TRACE_ERROR("%p (0x%x) not found in queue\n", this, 
                        ((INITIATOR_CMD_T*) this)->tag);
            ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
            return NULL;
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&q->lock, return NULL);
    return NULL;
}

/*
 * Hashing Functions 
 */

#include "initiator.h"
#include "target.h"

int hash_init(HASH_T *h, int n) {
    ISCSI_SPIN_LOCK_INIT_ELSE(&h->lock, return -1);
    h->n = n;
    h->insertions = 0;
    h->collisions = 0;
    if ((h->bucket=iscsi_malloc_atomic(n*sizeof(HASH_ELEM_T*)))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        ISCSI_SPIN_LOCK_DESTROY_ELSE(&h->lock, return -1);
        return -1;
    }
    memset(h->bucket, 0, n*sizeof(HASH_ELEM_T*));
    return 0;
}

int hash_insert(HASH_T *h, HASH_ELEM_T *elem) {
    int i;  

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return -1);
    i = (elem->key)%(h->n);
    if (h->bucket[i]==NULL) {
        TRACE(TRACE_HASH, 0, "inserting key 0x%x (ptr %p) into bucket[%i]\n", 
              elem->key, elem->ptr, i);
        h->bucket[i] = elem;
        elem->next = NULL;
    } else {
        HASH_ELEM_T *ptr = h->bucket[i];
  
        while (ptr) {
            if (elem->key == ptr->key) {
                TRACE_ERROR("REHASHING 0x%x\n", ptr->key)
                    return -1;
            }
            ptr = ptr->next;
        }
        elem->next = h->bucket[i];
        h->bucket[i] = elem;
        h->collisions++;
        TRACE(TRACE_HASH, 0, 
              "inserting key 0x%x (ptr %p) into bucket[%i] (COLLISION)\n", 
              elem->key, elem->ptr, i);
    }
    h->insertions++;
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return -1);

    return 0;
}

int hash_print(HASH_T *h) {
    int i;
    HASH_ELEM_T *curr;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return -1);
    PRINT("hash elements: ");
    for (i=0; i<h->n; i++) {
        curr = (HASH_ELEM_T *)(h->bucket[i]);
        while (curr) {
            PRINT("0x%x->%p,", curr->key, curr->ptr);
            curr = curr->next;
            if (curr == h->bucket[i]) break;
        }
    }
    PRINT("\n");
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return -1);

    return 0;

}

int hash_count(HASH_T *h) {
    int i;
    HASH_ELEM_T *curr;
    int count = 0;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return -1);
    for (i=0; i<h->n; i++) {
        curr = (HASH_ELEM_T *)(h->bucket[i]);
        while (curr) {
            count++;
            curr = curr->next;
            if (curr == h->bucket[i]) break;
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return -1);

    return count;
}

void *hash_get(HASH_T *h, unsigned key) {
    HASH_ELEM_T *prev, *curr;
    int i;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return NULL);
    i = key%(h->n);
    if (h->bucket[i]==NULL) {
        curr = NULL;
    } else {  
        prev = NULL;
        curr = h->bucket[i];
        while ((curr->key!=key)&&(curr->next!= NULL)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key!=key) {
            curr = NULL;
        } 
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
    if (curr) return curr->ptr;
    else return NULL;
}

HASH_ELEM_T *hash_get_elem(HASH_T *h, unsigned key) {
    HASH_ELEM_T *prev, *curr;
    int i;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return NULL);
    i = key%(h->n);
    if (h->bucket[i]==NULL) {
        curr = NULL;
    } else {  
        prev = NULL;
        curr = h->bucket[i];
        while ((curr->key!=key)&&(curr->next!= NULL)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key!=key) {
            curr = NULL;
        } 
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
    if (curr) return curr;
    else return NULL;
}


void *hash_remove_head(HASH_T *h) {
    HASH_ELEM_T *curr = NULL;
    int i;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return NULL);
    for (i=0; i<h->n; i++) {
        if (h->bucket[i]!=NULL) {           
            curr = h->bucket[i];
            h->bucket[i] = ((HASH_ELEM_T *)(h->bucket[i]))->next;
            ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
            return curr->ptr;
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
    return NULL;
}

void *hash_remove(HASH_T *h, unsigned key) {
    HASH_ELEM_T *prev, *curr;
    int i;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return NULL);
    i = key%(h->n);
    if (h->bucket[i]==NULL) {
        curr = NULL;
    } else {  
        prev = NULL;
        curr = h->bucket[i];
        while ((curr->key!=key)&&(curr->next!= NULL)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key!=key) {
            curr = NULL;
        } else {
            if (prev==NULL) {
                h->bucket[i] = ((HASH_ELEM_T *)(h->bucket[i]))->next;
                TRACE(TRACE_HASH, 0, 
                      "removed key 0x%x (ptr %p) from head of bucket\n", 
                      key, curr->ptr);
                TRACE(TRACE_HASH, 0, "next is %p\n", h->bucket[i]);
            } else {
                prev->next = curr->next;
                if (prev->next == NULL) {
                    TRACE(TRACE_HASH, 0, 
                          "removed key 0x%x (ptr %p) from end of bucket\n", 
                          key, curr->ptr);
                } else {
                    TRACE(TRACE_HASH, 0, 
                          "removed key 0x%x (ptr %p) from middle of bucket\n", 
                          key, curr->ptr);
                }
            }
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
    if (curr) return curr->ptr;
    else return NULL;
}

HASH_ELEM_T *hash_remove_elem(HASH_T *h, unsigned key) {
    HASH_ELEM_T *prev, *curr;
    int i;

    ISCSI_SPIN_LOCK_ELSE(&h->lock, return NULL);
    i = key%(h->n);
    if (h->bucket[i]==NULL) {
        curr = NULL;
    } else {  
        prev = NULL;
        curr = h->bucket[i];
        while ((curr->key!=key)&&(curr->next!= NULL)) {
            prev = curr;
            curr = curr->next;
        }
        if (curr->key!=key) {
            curr = NULL;
        } else {
            if (prev==NULL) {
                h->bucket[i] = ((HASH_ELEM_T *)(h->bucket[i]))->next;
                TRACE(TRACE_HASH, 0, 
                      "removed key 0x%x (ptr %p) from head of bucket\n", 
                      key, curr->ptr);
                TRACE(TRACE_HASH, 0, "next is %p\n", h->bucket[i]);
            } else {
                prev->next = curr->next;
                if (prev->next == NULL) {
                    TRACE(TRACE_HASH, 0, 
                          "removed key 0x%x (ptr %p) from end of bucket\n", 
                          key, curr->ptr);
                } else {
                    TRACE(TRACE_HASH, 0, 
                          "removed key 0x%x (ptr %p) from middle of bucket\n", 
                          key, curr->ptr);
                }
            }
        }
    }
    ISCSI_SPIN_UNLOCK_ELSE(&h->lock, return NULL);
    if (curr) return curr;
    else return NULL;
}

int hash_destroy(HASH_T *h) {
    h->n = 0;
    iscsi_free_atomic(h->bucket);
    ISCSI_SPIN_LOCK_DESTROY_ELSE(&h->lock, return -1);
    TRACE(TRACE_HASH, 0, "hash %p destroyed\n", h);
    return 0;
}

/*
 * Socket Functions
 */

int modify_iov(struct iovec **iov_ptr, int *iov_len, unsigned offset, 
               unsigned length) {
    int len;
    int disp = offset;
    int i;
    struct iovec* iov = *iov_ptr;

#ifdef CONFIG_ISCSI_DEBUG
    TRACE(TRACE_IOV, 0, "orig iov:\n");
    len = 0;
    for (i=0; i<*iov_len; i++) {
        TRACE(TRACE_IOV, 0, "iov[%i].iov_base = %p (len %Zu)\n", i, 
              iov[i].iov_base, iov[i].iov_len);
        len += iov[i].iov_len;
    }
    TRACE(TRACE_IOV, 0, "orig iov length: %u bytes\n", len);
#endif

    // Given <offset>, find beginning iovec and modify its base and length

    len = 0;
    for (i=0; i<*iov_len; i++) {
        len += iov[i].iov_len;
        if (len>offset) {
            TRACE(TRACE_IOV, 1, "found offset %u in iov[%i]\n", offset, i);
            break;
        }
        disp -= iov[i].iov_len;
    }
    if (i==*iov_len) {
        TRACE_ERROR("sum of iov lens (%u) < offset (%u)\n", len, offset);
        return -1;
    }

    iov[i].iov_len -= disp;
    iov[i].iov_base += disp;
    *iov_len -= i;
    *iov_ptr = &(iov[i]);
    iov = *iov_ptr;

    /* Given <length>, find ending iovec and modify its length 
     * (base does not change) */

    len = 0; // we should re-use len and i here...
    for (i=0; i<*iov_len; i++) {
        len += iov[i].iov_len;
        if (len>=length) {
            TRACE(TRACE_IOV, 1, "length %u ends in iovec[%i]\n", length, i);
            break;
        }
    }
    if (i==*iov_len) {
        TRACE_ERROR("sum of iovec lens (%u) < length (%u)\n", len, length);
        for (i=0; i<*iov_len; i++) {
            TRACE_ERROR("iov[%i].iov_base = %p (len %Zu)\n", i, 
                        iov[i].iov_base, iov[i].iov_len);
        }
        return -1;
    }
    iov[i].iov_len -= (len-length);
    *iov_len = i+1;

#ifdef CONFIG_ISCSI_DEBUG
    TRACE(TRACE_IOV, 0, "new iov:\n");
    len = 0;
    for (i=0; i<*iov_len; i++) {
        TRACE(TRACE_IOV, 0, "iov[%i].iov_base = %p (len %Zu)\n", i, 
              iov[i].iov_base, iov[i].iov_len);
        len += iov[i].iov_len;
    }
    TRACE(TRACE_IOV, 0, "new iov length: %u bytes\n", len);
#endif

    return 0;
}

int iscsi_sock_setsockopt(iscsi_socket_t *sock, int level, int optname, 
                          void *optval, unsigned optlen) {
    int rc;

#ifdef __KERNEL__
    mm_segment_t  oldfs;

    oldfs = get_fs(); 
    set_fs(KERNEL_DS);
    if (level == SOL_SOCKET) {
        if ((rc=sock_setsockopt(*sock, level, optname, (char *) optval, 
                                optlen))!=0) {
            TRACE_ERROR("sock_setsockopt() failed: rc %i\n", rc);
            set_fs(oldfs);
            return -1;
        }
    } else {
        if ((rc=(*sock)->ops->setsockopt(*sock, level, optname, (char *) 
                                         optval, optlen))!=0) {
            TRACE_ERROR("sock->setsockopt() failed: rc %i\n", rc);
            set_fs(oldfs);
            return -1;
        }
    }
    set_fs(oldfs);
#else
    if ((rc=setsockopt(*sock, level, optname, optval, optlen))!=0) {
        TRACE_ERROR("sock->ops->setsockopt() failed: rc %i errno %i\n", rc, 
                    errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_getsockopt(iscsi_socket_t *sock, int level, int optname, 
                          void *optval, unsigned *optlen) {
#ifdef __KERNEL__
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    TRACE_ERROR("iscsi_sock_getsockopt() not implemented for 2.6\n");
    return -1;
#else
    int rc;
    mm_segment_t  oldfs;

    oldfs = get_fs(); 
    set_fs(KERNEL_DS);
    if (level == SOL_SOCKET) {
        if ((rc=sock_getsockopt(*sock, level, optname, optval, optlen))!=0) {
            TRACE_ERROR("sock_getsockopt() failed: rc %i\n", rc);
            set_fs(oldfs);
            return -1;
        }
    } else {
        if ((rc=(*sock)->ops->getsockopt(*sock, level, optname, optval, 
                                         optlen))!=0) {
            TRACE_ERROR("sock->getsockopt() failed: rc %i\n", rc);
            set_fs(oldfs);
            return -1;
        }
    }
    set_fs(oldfs);
#endif
#else
    int rc;
    if ((rc=getsockopt(*sock, level, optname, optval, optlen))!=0) {
        TRACE_ERROR("sock->ops->getsockopt() failed: rc %i errno %i\n", 
                    rc, errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_create(iscsi_socket_t *sock, int family) {
    int rc;

#ifdef __KERNEL__
    if ((rc=sock_create(family, SOCK_STREAM, IPPROTO_TCP, sock))<0) {
        TRACE_ERROR("sock_create() failed: rc %i\n", rc);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    (*sock)->sk->sk_allocation = GFP_NOIO;
#else
    (*sock)->sk->allocation = GFP_NOIO;
#endif

#else
    if ((rc=socket(family, SOCK_STREAM, 0))<0) {
        TRACE_ERROR("socket() failed: rc %i errno %i\n", rc, errno);
    }
    *sock = rc;
#if 0
    {
        int recvsize = 65536;
        int sendsize = 32768;
        int size = sizeof(int);
        int err;

        setsockopt(*sock, SOL_SOCKET, SO_SNDBUF, (char *)&sendsize, 
                   sizeof(sendsize));
        setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, (char *)&recvsize, 
                   sizeof(recvsize));
        err = getsockopt(*sock, SOL_SOCKET, SO_SNDBUF, (char *) &sendsize, 
                         &size);
        PRINT("send buffer size = %i\n", sendsize);
        err = getsockopt(*sock, SOL_SOCKET, SO_RCVBUF, (char *) &recvsize, 
                         &size);
        PRINT("recv buffer size = %i\n", recvsize);
    }
#endif
#endif
    if (rc<0) {
        TRACE_ERROR("error creating socket (rc %i)\n", rc);
        return -1;
    }
    return 0;
}

int iscsi_sock_bind(iscsi_socket_t sock, int port, int family) {
    struct sockaddr_in laddr;
    int rc;

    memset((char *)&laddr, 0, sizeof(laddr));
    laddr.sin_family = family;
    laddr.sin_addr.s_addr  = INADDR_ANY;
    laddr.sin_port = HTONS(port);
#ifdef __KERNEL__
    if ((rc=sock->ops->bind(sock, (struct sockaddr*) &laddr, 
                            sizeof(laddr)))<0) {
        TRACE_ERROR("sock->ops->bind() failed: rc %i\n", rc);
        return -1;
    }
#else
    if ((rc=bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)))<0) {
        TRACE_ERROR("bind() failed: rc %i errno %i\n", rc, errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_listen(iscsi_socket_t sock) {
    int rc;

#ifdef __KERNEL__
    if ((rc=sock->ops->listen(sock, 32))<0) {
        TRACE_ERROR("sock->ops->listen() failed: rc %i\n", rc);
        return -1;
    }
#else
    if ((rc=listen(sock, 32))<0) {
        TRACE_ERROR("listen() failed: rc %i errno %i\n", rc, errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_accept(iscsi_socket_t sock, iscsi_socket_t *newsock) {
#ifdef __KERNEL__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    int rc;
#else
#endif
#else
    struct sockaddr_in remoteAddr;
    socklen_t remoteAddrLen;
#endif

#ifdef __KERNEL__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    *newsock = sock_alloc();
    (*newsock)->ops = sock->ops;
    (*newsock)->type = sock->type;
    if ((rc=(*newsock)->ops->accept(sock, *newsock, 0))<0) {
        TRACE(TRACE_NET, 0, "newsock->ops->accept() failed: rc %i\n", rc);
        return -1;
    }
#endif
#else
    remoteAddrLen = sizeof(remoteAddr);
    memset((char *)&remoteAddr, 0, sizeof(remoteAddr));
    if ((*newsock = accept(sock, (struct sockaddr *) &remoteAddr, 
                           &remoteAddrLen))<0) {
        TRACE(TRACE_NET, 0, "accept() failed: rc %i errno %i\n", *newsock, 
              errno);
        return -1;
    }
#endif

#if 0
#if (BYTE_ORDER == BIG_ENDIAN)
    sprintf(local, "%u.%u.%u.%u", 
            (localAddr.sin_addr.s_addr&0xff000000)>>24,
            (localAddr.sin_addr.s_addr&0x00ff0000)>>16,
            (localAddr.sin_addr.s_addr&0x0000ff00)>>8,
            (localAddr.sin_addr.s_addr&0x000000ff));
    sprintf(remote, "%u.%u.%u.%u", 
            (remoteAddr.sin_addr.s_addr&0xff000000)>>24,
            (remoteAddr.sin_addr.s_addr&0x00ff0000)>>16,
            (remoteAddr.sin_addr.s_addr&0x0000ff00)>>8,
            (remoteAddr.sin_addr.s_addr&0x000000ff));
#else
    sprintf(local, "%u.%u.%u.%u", 
            (localAddr.sin_addr.s_addr&0x000000ff),
            (localAddr.sin_addr.s_addr&0x0000ff00)>>8,
            (localAddr.sin_addr.s_addr&0x00ff0000)>>16,
            (localAddr.sin_addr.s_addr&0xff000000)>>24);
    sprintf(remote, "%u.%u.%u.%u", 
            (remoteAddr.sin_addr.s_addr&0x000000ff),
            (remoteAddr.sin_addr.s_addr&0x0000ff00)>>8,
            (remoteAddr.sin_addr.s_addr&0x00ff0000)>>16,
            (remoteAddr.sin_addr.s_addr&0xff000000)>>24);
#endif
#endif

    return 0;
}

int iscsi_sock_getsockname(iscsi_socket_t sock, struct sockaddr *name, 
                           unsigned *namelen) {
#ifdef __KERNEL__
    int rc;

    if ((rc=sock->ops->getname(sock, name, namelen, 0))!=0) {
        TRACE_ERROR("sock->ops->getname() failed: rc %i\n", rc);
        return -1;
    }
#else
    if (getsockname(sock, name, namelen)!=0) {
        TRACE_ERROR("getsockame() failed (errno %i)\n", errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_getpeername(iscsi_socket_t sock, struct sockaddr *name, 
                           unsigned *namelen) {
#ifdef __KERNEL__
    int rc;

    if ((rc=sock->ops->getname(sock, name, namelen, 1))!=0) {
        TRACE_ERROR("sock->ops->getname() failed: rc %i\n", rc);
        return -1;
    }
#else
    if (getpeername(sock, name, namelen)!=0) {
        TRACE_ERROR("getpeername() failed (errno %i)\n", errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_shutdown(iscsi_socket_t sock, int how) {
    int rc;

#ifdef __KERNEL__
    rc = sock->ops->shutdown(sock, 
                             (how==ISCSI_SOCK_SHUTDOWN_RECV)?
                             RCV_SHUTDOWN:SEND_SHUTDOWN);
#else
    rc = shutdown(sock, (how==ISCSI_SOCK_SHUTDOWN_RECV)?SHUT_RD:SHUT_WR);
#endif
    if (rc != 0) {
        TRACE(TRACE_NET, 0, "sock_shutdown failed (errno %i)\n", rc);
    }
    return 0;
}

int iscsi_sock_close(iscsi_socket_t sock) {
#ifdef __KERNEL__
    sock_release(sock);
#else
    int rc;
    if ((rc=close(sock))!=0) {
        TRACE_ERROR("close() failed: rc %i errno %i\n", rc, errno);
        return -1;
    }
#endif
    return 0;
}

int iscsi_sock_connect(iscsi_socket_t sock, char *hostname, int port, 
                       int family) {
#ifdef __KERNEL__
    __u32 in_aton(const char *str);
    int errno = 0;
#endif
    struct sockaddr_in addr;
    int rc = 0;
    int i;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = family;
    addr.sin_port = HTONS(port);
    addr.sin_family = family;

    for (i=0; i<ISCSI_SOCK_CONNECT_TIMEOUT; i++) {

        // Attempt connection

#ifdef __KERNEL__
        addr.sin_addr.s_addr = in_aton(hostname);
#if (ISCSI_SOCK_CONNECT_NONBLOCK==1)
        rc = sock->ops->connect(sock, (struct sockaddr *) &addr, sizeof(addr), 
                                O_NONBLOCK);
#else
        rc = sock->ops->connect(sock, (struct sockaddr *) &addr, sizeof(addr), 
                                0);
#endif
        if (rc<0) {
            errno = rc*-1;
            rc = -1;
        }
#else
        addr.sin_addr.s_addr = inet_addr(hostname);
#if (ISCSI_SOCK_CONNECT_NONBLOCK==1)
        if (fcntl(sock, F_SETFL, O_NONBLOCK)!=0) {
            TRACE_ERROR("fcntl() failed");
            return -1;
        }
#endif
        rc = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
#if (ISCSI_SOCK_CONNECT_NONBLOCK==1)
        if (fcntl(sock, F_SETFL, O_SYNC)!=0) {
            TRACE_ERROR("fcntl() failed\n");
            return -1;
        }
#endif
#endif

        // Check errno

        if (errno==EISCONN) {
            rc = 0;
            break;
        }
        if ((errno==EAGAIN)||(errno==EINPROGRESS)||(errno==EALREADY)) {
            if (i!=(ISCSI_SOCK_CONNECT_TIMEOUT-1)) {
                ISCSI_SLEEP(1);
            }
        } else {
            break;
        }
    }
    return rc;
}

/*
 * NOTE: iscsi_sock_msg() alters *sg when socket sends and recvs return 
 * having only transfered a portion of the iovec.  When this happens, the 
 * iovec is modified and resent with the appropriate offsets.
 */

#define RESTORE_IOV

int iscsi_sock_msg(iscsi_socket_t sock, int send, int len, void *data, 
                   int iov_len) {
    int i, rc, n = 0;
    struct iovec *iov;
    struct iovec singleton;
#ifdef __KERNEL__
    struct msghdr msg;
    mm_segment_t  oldfs;
#endif
    struct iovec *iov_orig;
    unsigned char padding[ISCSI_SOCK_MSG_BYTE_ALIGN];
    struct iovec *iov_padding = NULL;
    unsigned remainder;
    unsigned padding_len = 0;
    int total_len = 0;

    TRACE(TRACE_NET, 0, "%s %i bytes on sock\n", send?"sending":"receiving", 
          len);
    if (!iov_len) {
        TRACE(TRACE_NET, 1, "building singleton iovec (data %p, len %u)\n", 
              data, len);
        singleton.iov_base = data;
        singleton.iov_len = len;
        iov = &singleton; 
        iov_len = 1;
    } else {
        iov = (struct iovec *) data;
    }

    // Add padding

    if ((remainder=len%ISCSI_SOCK_MSG_BYTE_ALIGN)) {
        TRACE(TRACE_MEM, 0, "malloc padding %u\n", (unsigned) ((iov_len+1)*sizeof(struct iovec)));
        if ((iov_padding=iscsi_malloc_atomic((iov_len+1)*
                                             sizeof(struct iovec)))==NULL) {
            TRACE_ERROR("iscsi_malloc_atomic() failed\n");
            return -1;
        }
        memcpy(iov_padding, iov, iov_len*sizeof(struct iovec));
        iov_padding[iov_len].iov_base = padding; 
        padding_len = ISCSI_SOCK_MSG_BYTE_ALIGN - remainder;
        iov_padding[iov_len].iov_len = padding_len;
        iov = iov_padding;
        iov_len++;
        memset(padding, 0, padding_len);
        len += padding_len;
        TRACE(TRACE_NET, 1, "Added iovec for padding (len %u)\n", padding_len);
    }

#ifdef RESTORE_IOV
    /* Make a copy of the iovec so that we can restore the original
     * in case it gets modified by partial sends/recvs */
    TRACE(TRACE_MEM, 0, "malloc iov_orig %u\n", (unsigned) (iov_len*sizeof(struct iovec)));
    if ((iov_orig=iscsi_malloc_atomic(iov_len*sizeof(struct iovec)))==NULL) {
        TRACE_ERROR("iscsi_malloc_atomic() failed\n");
        return -1;
    }
    memcpy(iov_orig, iov, iov_len*sizeof(struct iovec));
#endif
    
    do {

        // Check iovec

        total_len = 0;
        TRACE(TRACE_NET, 0, "%s %i buffers\n", 
              send?"gathering from":"scattering into", iov_len);
        for (i=0; i<iov_len; i++) {
            if (iov[i].iov_base == NULL) {
                TRACE_ERROR("iov[%i].iov_base = %p, len %Zu\n", 
                            i, iov[i].iov_base, iov[i].iov_len);
                return -1;
            }
            TRACE(TRACE_NET, 0, "iov[%i].iov_base = %p, len %Zu\n", 
                  i, iov[i].iov_base, iov[i].iov_len);
            total_len += iov[i].iov_len;
        }
        if (total_len != len-n) {
            TRACE_ERROR("iov_lens sum to %i != total len of %i\n", 
                        total_len, len-n);
#if 0
            for (i=0; i<iov_len; i++) {
                TRACE_ERROR("iov[%i].iov_base = %p, len %u\n", 
                            i, iov[i].iov_base, iov[i].iov_len);
            }
#endif
            return -1;
        }

#ifdef __KERNEL__
        oldfs = get_fs(); 
        set_fs(KERNEL_DS);
        msg.msg_iov = iov;
        msg.msg_iovlen = iov_len;
        msg.msg_name = NULL; msg.msg_namelen = 0;
        msg.msg_control = NULL; msg.msg_controllen = 0;
        msg.msg_flags = 0;
        if (send) {
            rc = sock_sendmsg(sock, &msg, len-n);
        } else {
            rc = sock_recvmsg(sock, &msg, len-n, 0);
        }
        set_fs(oldfs);
#else
        if (send) {
            rc = writev(sock, iov, iov_len);
        } else {
            rc = readv(sock, iov, iov_len);
        }
#endif

        if (rc<=0) {
#ifdef __KERNEL__
            if (rc==0) {
                TRACE(TRACE_NET, 1, "%s() failed: rc %i\n", 
                      send?"sock_sendmsg":"sock_recvmsg", rc);
            } else {
                TRACE_ERROR("%s() failed: rc %i\n", 
                            send?"sock_sendmsg":"sock_recvmsg", rc);
            }
#else
            if (rc==0) {
                TRACE(TRACE_NET, 1, "%s() failed: rc %i errno %i\n", 
                      send?"writev":"readv", rc, errno);
            }
#endif
            break;
        }
        n += rc;
        if (n<len) {
            TRACE(TRACE_NET, 1, "Got partial %s: %i bytes of %i\n", 
                  send?"send":"recv", rc, len-n+rc);

            // sock_sendmsg|recvmsg automatically update the iovec.

#ifndef __KERNEL__
            total_len = 0; 
            for (i=0; i<iov_len; i++) total_len += iov[i].iov_len;
            TRACE(TRACE_NET, 0, 
                  "before modify_iov: %s %i buffers, total_len = %u, "
                  "n = %u, rc = %u\n", 
                  send?"gathering from":"scattering into", 
                  iov_len, total_len, n, rc);
            if (modify_iov(&iov, &iov_len, rc, len-n)!=0) {
                TRACE_ERROR("modify_iov() failed\n");
                break;
            }
            total_len = 0; for (i=0; i<iov_len; i++)  
                               total_len += iov[i].iov_len;
            TRACE(TRACE_NET, 0, 
                  "after modify_iov: %s %i buffers, total_len = %u, "
                  "n = %u, rc = %u\n\n", 
                  send?"gathering from":"scattering into", 
                  iov_len, total_len, n, rc);
#endif
        }
    } while (n<len);

#ifdef RESTORE_IOV
    /* restore iov to its original self */
    memcpy(iov, iov_orig, iov_len*sizeof(struct iovec));
#endif

#ifdef CONFIG_ISCSI_DEBUG
    if (n==len) {
        for (i=0; i<iov_len; i++) {
            TRACE(TRACE_NET, 1, "iovec[%i] len %Zu:\n", i, iov_orig[i].iov_len);
            PRINT_BUFF(TRACE_NET, 1, (unsigned char *)iov_orig[i].iov_base, 
                       iov_orig[i].iov_len, 32);
        }
    }
#endif

#ifdef RESTORE_IOV
    iscsi_free_atomic(iov_orig);
#endif

    if (remainder) iscsi_free_atomic(iov_padding);
    TRACE(TRACE_NET, 0, "successfully %s %i bytes on sock (%i bytes padding)\n",
          send?"sent":"received", n, padding_len);


    return n-padding_len;
}

/*
 * Temporary Hack:
 *
 * TCP's Nagle algorithm and delayed-ack lead to poor performance when we send
 * two small messages back to back (i.e., header+data). The TCP_NODELAY option
 * is supposed to turn off Nagle, but it doesn't seem to work on Linux 2.4.
 * Because of this, if our data payload is small, we'll combine the header and
 * data, else send as two separate messages.
 */

int iscsi_sock_send_header_and_data(iscsi_socket_t sock,
                                    void *header, int header_len,
                                    void *data, int data_len, int iov_len) {
    struct iovec *iov;
    int rc;
#ifdef __KERNEL__
    int errno = 0;
#endif

    if (data_len&&(data_len<=ISCSI_SOCK_HACK_CROSSOVER)) {
        TRACE(TRACE_NET, 1, 
              "combining header and data (iov_len %i) into one iovec\n", 
              iov_len);
        if (iov_len) {
            TRACE(TRACE_MEM, 0, "malloc iov %u\n", (unsigned) (sizeof(struct iovec)*(iov_len+1)));
            if ((iov=
                 iscsi_malloc_atomic(sizeof(struct iovec)*(iov_len+1)))==NULL) {
                TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                return -1;
            }
            iov[0].iov_base = header; iov[0].iov_len = header_len;
            memcpy(&(iov[1]), data, sizeof(struct iovec)*iov_len);
            iov_len++;
        } else {
            TRACE(TRACE_MEM, 0, "malloc iov %u\n", (unsigned) (sizeof(struct iovec)*(2)));
            if ((iov=iscsi_malloc_atomic(sizeof(struct iovec)*(2)))==NULL) {
                TRACE_ERROR("iscsi_malloc_atomic() failed\n");
                return -1;
            }
            iov[0].iov_base = header; iov[0].iov_len = header_len;
            iov[1].iov_base = data; iov[1].iov_len = data_len;
            iov_len = 2;
        }
        if ((rc=iscsi_sock_msg(sock, 1, header_len+data_len, iov, iov_len))!=
            header_len+data_len) {
            TRACE_ERROR("iscsi_sock_msg() failed: got %i, expected %i\n", 
                        rc, header_len+data_len);
            return -1;
        }
        iscsi_free_atomic(iov);
    } else {
        if ((rc=iscsi_sock_msg(sock, 1, header_len, header, 0))!=header_len) {
            TRACE_ERROR("iscsi_sock_msg() failed (errno %i rc %i)\n", errno, 
                        rc);
            return -1;
        }
        if (data_len) {
            if (iscsi_sock_msg(sock, 1, data_len, data, iov_len)!=data_len) {
                TRACE_ERROR("iscsi_sock_msg() failed\n");
                return -1;
            }
        }
    }
    return header_len+data_len;
}

/*
 * Locks
 */

int iscsi_lock_init(ISCSI_LOCK_T *l) {
#ifdef __KERNEL__
    //init_MUTEX(l);
    sema_init(l, 1);
#else
    if (pthread_mutex_init(l, NULL)!=0) {
        return -1;
    }
#endif
    return 0;
}

int iscsi_lock_ni(ISCSI_LOCK_T *l) {
#ifdef __KERNEL__
    if (in_interrupt()) {
#if 0
        if (!atomic_read(&l->count)) {
            TRACE_ERROR("semaphore will block within an interrupt!\n");
            return -1;
        }
#endif
    }
    down(l);
    return 0;
#else
    return iscsi_lock(l); 
#endif
}

int iscsi_lock(ISCSI_LOCK_T *l) {
#ifdef __KERNEL__
    int attempt = 0;

    if (in_interrupt()) {
#if 0
        if (!atomic_read(&l->count)) {
            TRACE_ERROR("semaphore will block within an interrupt!\n");
            return -1;
        }
#endif
    }
    while (down_interruptible(l)!=0) {
        TRACE_ERROR("pid %i: semaphore not acquired (attempt %i)\n", 
                    OST_GETPID, attempt);
        ISCSI_SLEEP(1);
        if (attempt > 10) {
            TRACE_ERROR("giving up on semaphore after %i attempts\n", attempt);
            return -1;
        }
        attempt++;
    }

    return 0;
#else
#if 0
    int backoff = 1;
    while (pthread_mutex_trylock(l)!=0) {
        usleep(backoff);
        backoff++;
        if (backoff>1024) return -1;
    }
    return 0;
#else
    return pthread_mutex_lock(l);
#endif
#endif
}

int iscsi_unlock(ISCSI_LOCK_T *l) {
#ifdef __KERNEL__
    up(l);
    return 0;
#else
    return pthread_mutex_unlock(l);
#endif
}

int iscsi_lock_destroy(ISCSI_LOCK_T *l) {
#ifdef __KERNEL__
    return 0;
#else
    return pthread_mutex_destroy(l);
#endif
}

/*
 * spin locks (only used apply for kernel mode)
 */

int iscsi_spin_lock_init(ISCSI_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    l->lock = SPIN_LOCK_UNLOCKED;
    return 0;
#else
    return iscsi_lock_init(l);
#endif
}

int iscsi_spin_lock(ISCSI_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    spin_lock_irqsave(&l->lock, l->flags);
    return 0;
#else
    return iscsi_lock(l);
#endif
}

int iscsi_spin_unlock(ISCSI_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    spin_unlock_irqrestore(&l->lock, l->flags);
    return 0;
#else
    return iscsi_unlock(l);
#endif
}

int iscsi_spin_lock_destroy(ISCSI_SPIN_LOCK_T *l) {
#ifdef __KERNEL__
    return 0;
#else  
    return iscsi_lock_destroy(l);
#endif
}

/*
 * Misc. Functions
 */

unsigned iscsi_atoi(char *value) {
#ifdef __KERNEL__
    char *trash;
#endif
    if (value==NULL) {
        TRACE_ERROR("iscsi_atoi() called with NULL value\n");
        return 0;
    }
#ifdef __KERNEL__
    return simple_strtoul(value, &trash, 10);
#else
    return atoi(value);
#endif
}

static const char HexString[] = "0123456789abcdefABCDEF";
int HexStringIndex(const char *s, int c)
{
    int n = 0;

    while (*s != '\0') {
        if (*s++ == c) {
            return n;
        }
        n++;
    }

    return -1;
}

int HexDataToText(
                  unsigned char *data, unsigned int dataLength,
                  char *text, unsigned int textLength)

{
    unsigned long n;

    if (!text || textLength == 0) {
        return -1;
    }

    if (!data || dataLength == 0) {
        *text = '\0';
        return -1;
    }

    if (textLength < 3) {
        *text = '\0';
        return -1;
    }

    *text++ = '0';
    *text++ = 'x';

    textLength -= 2;

    while (dataLength > 0) {

        if (textLength < 3) {
            *text = '\0';
            return -1;
        }

        n = *data++;
        dataLength--;

        *text++ = HexString[(n >> 4) & 0xf];
        *text++ = HexString[n & 0xf];

        textLength -= 2;
    }
    *text = '\0';

    return dataLength;
}


int HexTextToData(
                  const char *text, unsigned int textLength,
                  unsigned char *data, unsigned int dataLength)

{
    int i;
    unsigned int n1;
    unsigned int n2;
    unsigned int len = 0;

    if ((text[0] == '0') && (text[1] != 'x' || text[1] != 'X')) {
        /* skip prefix */
        text += 2;
        textLength -= 2;
    }
    if ((textLength % 2) == 1) {

        i = HexStringIndex(HexString, *text++);
        if (i < 0) return -1; /* error, bad character */

        if (i > 15) i -= 6;
        n2 = i;

        if (dataLength < 1) {
            TRACE_ERROR("too much data\n");
            return -1; /* error, too much data */
        }

        *data++ = n2;
        len++;
    }

    while (*text != '\0') {

        i = HexStringIndex(HexString, *text++);
        if (i < 0) return -1; /* error, bad character */

        if (i > 15) i -= 6;
        n1 = i;

        if (*text == '\0') {
            TRACE_ERROR("odd string length\n");
            return -1; /* error, odd string length */
        }

        i = HexStringIndex(HexString, *text++);
        if (i < 0) {
            TRACE_ERROR("bad character\n");
            return -1; /* error, bad character */
        }

        if (i > 15) i -= 6;
        n2 = i;

        if (len >= dataLength) {
            TRACE_ERROR("too much data\n");
            return -1; /* error, too much data */
        }

        *data++ = (n1 << 4) | n2;
        len++;
    }

    if (len == 0) {
        TRACE_ERROR("no data\n");
        return -1; /* error, no data */
    }

    return len; /* no error */
}

void GenRandomData(unsigned char *data, unsigned int length)
{
#if defined(__KERNEL__)
    get_random_bytes(data, length);
#else
    long r;
    unsigned n;

    while (length > 0) {

        r = rand();
        r = r ^ (r >> 8);
        r = r ^ (r >> 4);
        n = r & 0x7;

        r = rand();
        r = r ^ (r >> 8);
        r = r ^ (r >> 5);
        n = (n << 3) | (r & 0x7);

        r = rand();
        r = r ^ (r >> 8);
        r = r ^ (r >> 5);
        n = (n << 2) | (r & 0x3);

        *data++ = n;
        length--;
    }
#endif
}

/*
 * run a command and read its output from a pipe
 */
#ifndef __KERNEL__
int command_pipe(char *cmd, char *buffer, int len) {
    int pfd[2];
    int pid;
    int rc;

    if (pipe(pfd) == -1) {
        perror("pipe failed");
        return -1;
    }
    if ((pid = fork()) < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        close(pfd[0]); 
        dup2(pfd[1], 1); 
        close(pfd[1]);
        printf("cmd = \"%s\"\n", cmd);
        rc = execlp(cmd, "foo", (char *) 0);
        printf("rc = %i\n", rc);
    } else {
        close(pfd[1]);
        dup2(pfd[0], 0);
        rc = read(pfd[0], buffer, len);
        close(pfd[0]);
    }
    return 0;
}
#endif











