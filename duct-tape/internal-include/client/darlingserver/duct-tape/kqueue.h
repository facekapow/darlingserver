#ifndef _DARLINGSERVER_CLIENT_DUCT_TAPE_KQUEUE_H_
#define _DARLINGSERVER_CLIENT_DUCT_TAPE_KQUEUE_H_

#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/queue.h>

#include <libsimple/lock.h>

typedef struct dserver_kqueue {
	LIST_ENTRY(dserver_kqueue) list_link;
	int epoll_fd;
	int wakeup_fd;
	struct kqfile xnu_kqfile;
} dserver_kqueue_t;

typedef LIST_HEAD(dserver_kqueue_list_head, dserver_kqueue) dserver_kqueue_list_head_t;

typedef struct dserver_klist {
	LIST_ENTRY(dserver_klist) list_link;
	os_refcnt_t refcnt;
	int fd;
	libsimple_lock_t lock;
	struct klist xnu_klist;
} dserver_klist_t;

typedef LIST_HEAD(dserver_klist_list_head, dserver_klist) dserver_klist_list_head_t;

dserver_kqueue_t* dserver_kqueue_for_xnu_kqueue(struct kqueue* xnu_kqueue);

#endif // _DARLINGSERVER_CLIENT_DUCT_TAPE_KQUEUE_H_
