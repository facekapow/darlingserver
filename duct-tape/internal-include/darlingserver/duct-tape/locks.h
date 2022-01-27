#ifndef _DARLINGSERVER_DUCT_TAPE_LOCKS_H_
#define _DARLINGSERVER_DUCT_TAPE_LOCKS_H_

#include <stdint.h>
#include <sys/queue.h>

#include <libsimple/lock.h>

#ifndef DSERVER_CLIENT
typedef struct dtape_mutex_link {
	TAILQ_ENTRY(dtape_mutex_link) link;
} dtape_mutex_link_t;

typedef TAILQ_HEAD(dtape_mutex_head, dtape_mutex_link) dtape_mutex_head_t;

typedef struct dtape_mutex {
	volatile uintptr_t dtape_owner;
	libsimple_lock_t dtape_queue_lock;
	dtape_mutex_head_t dtape_queue_head;
} dtape_mutex_t;
#endif // !DSERVER_CLIENT

typedef struct lck_mtx {
#ifdef DSERVER_CLIENT
	// padded to be pointer-sized
	libsimple_lock_t dtape_lock;
	uint8_t dtape_padding[sizeof(void*) - sizeof(libsimple_lock_t)];
#else
	// the actual mutex is allocated separately because the lck_mtx structure needs to be pointer-sized
	// for compatibility with the actual XNU lck_mtx structure (particularly necessary for XNU waitqs).
	dtape_mutex_t* dtape_mutex;
#endif // DSERVER_CLIENT
} lck_mtx_t;

typedef struct lck_spin {
	lck_mtx_t dtape_interlock;
} lck_spin_t;

#endif // _DARLINGSERVER_DUCT_TAPE_LOCKS_H_
