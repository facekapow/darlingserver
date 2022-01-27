#include <darlingserver/kqueue.h>
#include <elfcalls.h>
#include <client/darlingserver/duct-tape/kqueue.h>
#include <sys/proc_internal.h>
#include <sys/filedesc.h>
#include <libsimple/lock.h>
#include <sys/epoll.h>
#include <sys/sysproto.h>
#include <sys/event.h>
#include <sys/eventfd.h>

extern struct elf_calls* _elfcalls;
extern long close_internal(int fd);
extern void knote_post(struct knote *kn, long hint);

static struct filedesc this_desctable;
static struct proc this_proc = {
	.p_fd = &this_desctable,
};
static libsimple_once_t initialize_process_token = LIBSIMPLE_ONCE_INITIALIZER;

static libsimple_lock_t kqueue_list_lock = LIBSIMPLE_LOCK_INITIALIZER;
static dserver_kqueue_list_head_t kqueue_list_head = LIST_HEAD_INITIALIZER(&kqueue_list_head);

static libsimple_lock_t klist_list_lock = LIBSIMPLE_LOCK_INITIALIZER;
static dserver_klist_list_head_t klist_list_head = LIST_HEAD_INITIALIZER(&klist_list_head);

dserver_kqueue_t* dserver_kqueue_for_xnu_kqueue(struct kqueue* xnu_kqueue) {
	struct kqfile* kqfile = __container_of(xnu_kqueue, struct kqfile, kqf_kqueue);
	return __container_of(kqfile, dserver_kqueue_t, xnu_kqfile);
};

dserver_kqueue_t* dserver_kqueue_for_fd(int fd) {
	dserver_kqueue_t* dkqueue = NULL;

	libsimple_lock_lock(&kqueue_list_lock);
	LIST_FOREACH(dkqueue, &kqueue_list_head, list_link) {
		if (dkqueue->epoll_fd == fd) {
			break;
		}
	}
	libsimple_lock_unlock(&kqueue_list_lock);

	return dkqueue;
};

void dserver_klist_retain(dserver_klist_t* dklist) {
	os_ref_retain(&dklist->refcnt);
};

void dserver_klist_release(dserver_klist_t* dklist) {
	if (os_ref_release(&dklist->refcnt) > 0) {
		return;
	}

	// no need to acquire the lock here; no one else can be holding a reference to the list

	while (!SLIST_EMPTY(&dklist->xnu_klist)) {
		knote_detach(&dklist->xnu_klist, SLIST_FIRST(&dklist->xnu_klist));
	}

	LIST_REMOVE(dklist, list_link);

	_elfcalls->free(dklist);
};

dserver_klist_t* dserver_klist_for_fd(int fd, bool retain) {
	dserver_klist_t* dklist = NULL;

	libsimple_lock_lock(&kqueue_list_lock);
	LIST_FOREACH(dklist, &klist_list_head, list_link) {
		if (dklist->fd == fd) {
			break;
		}
	}

	if (dklist && retain) {
		dserver_klist_retain(dklist);
	}

	libsimple_lock_unlock(&kqueue_list_lock);

	return dklist;
};

static void initialize_process(void* context) {
	lck_mtx_init(&this_proc.p_fdmlock, LCK_GRP_NULL, LCK_ATTR_NULL);
	lck_mtx_init(&this_desctable.fd_kqhashlock, LCK_GRP_NULL, LCK_ATTR_NULL);
	lck_mtx_init(&this_desctable.fd_knhashlock, LCK_GRP_NULL, LCK_ATTR_NULL);
};

void dserver_kqueue_atfork_child(void) {

};

int dserver_kqueue_create(void) {
	int ret = -ENOMEM;
	libsimple_once(&initialize_process_token, initialize_process, NULL);

	dserver_kqueue_t* dkqueue = _elfcalls->calloc(1, sizeof(dserver_kqueue_t));
	if (!dkqueue) {
		goto out;
	}

	dkqueue->epoll_fd = -1;
	dkqueue->wakeup_fd = -1;

	ret = __linux_epoll_create1(EPOLL_CLOEXEC);
	if (ret < 0) {
		goto out;
	}
	dkqueue->epoll_fd = ret;

	ret = __linux_eventfd(0, EFD_CLOEXEC);
	if (ret < 0) {
		goto out;
	}
	dkqueue->wakeup_fd = ret;

	struct epoll_event settings;
	settings.events = EPOLLIN;
	settings.data.ptr = dkqueue;

	ret = __linux_epoll_ctl(dkqueue->epoll_fd, EPOLL_CTL_ADD, dkqueue->wakeup_fd, &settings);
	if (ret < 0) {
		goto out;
	}

	lck_spin_init(&dkqueue->xnu_kqfile.kqf_kqueue.kq_lock, LCK_GRP_NULL, LCK_ATTR_NULL);

	ret = dkqueue->epoll_fd;

out:
	if (ret < 0) {
		if (dkqueue) {
			if (dkqueue->wakeup_fd) {
				close_internal(dkqueue->wakeup_fd);
			}

			if (dkqueue->epoll_fd) {
				close_internal(dkqueue->epoll_fd);
			}

			_elfcalls->free(dkqueue);
		}
	}
	return ret;
};

struct knote* knote_alloc(void) {
	return _elfcalls->calloc(1, sizeof(struct knote));
};

void knote_free(struct knote* kn) {
	_elfcalls->free(kn);
};

int dserver_kqueue_kevent64(int kq, const struct kevent64_s* changelist, int nchanges, struct kevent64_s* eventlist, int nevents, unsigned int flags, const struct timespec* timeout) {
	struct kevent64_args args = {
		.fd = kq,
		.changelist = (uintptr_t)changelist,
		.nchanges = nchanges,
		.eventlist = (uintptr_t)eventlist,
		.nevents = nevents,
		.flags = flags,
		.timeout = (uintptr_t)timeout,
	};

	int ret = 0;
	int err = kevent64(&this_proc, &args, &ret);

	return (err) ? -err : ret;
};

int dserver_kqueue_kevent(int	kq, const struct kevent* changelist, int nchanges, struct	kevent* eventlist, int nevents, const struct timespec* timeout) {
	struct kevent_args args = {
		.fd = kq,
		.changelist = (uintptr_t)changelist,
		.nchanges = nchanges,
		.eventlist = (uintptr_t)eventlist,
		.nevents = nevents,
		.timeout = (uintptr_t)timeout,
	};

	int ret = 0;
	int err = kevent(&this_proc, &args, &ret);

	return (err) ? -err : ret;
};

wait_result_t dserver_kqueue_sleep(struct kqueue* kq, int flags, kevent_ctx_t kectx) {
	dserver_kqueue_t* dkqueue = dserver_kqueue_for_xnu_kqueue(kq);
	struct epoll_event events[4];

	int ret = __linux_epoll_wait(dkqueue->epoll_fd, events, sizeof(events) / sizeof(*events), (flags & KEVENT_FLAG_POLL) ? 0 : ((int)kectx->kec_deadline ? kectx->kec_deadline : -1));

	if (ret == 0) {
		return THREAD_TIMED_OUT;
	} else if (ret < 0) {
		if (ret == -EINTR) {
			return THREAD_INTERRUPTED;
		} else {
			return THREAD_RESTART;
		}
	}

	for (size_t i = 0; i < ret; ++i) {
		struct epoll_event* event = &events[i];

		if (event->data.ptr == dkqueue) {
			// this is the wakeup fd
		} else {
			// this is pointing to a knote list; give it the event in the hint
			dserver_klist_t* dklist = event->data.ptr;

			libsimple_lock_lock(&dklist->lock);
			knote(&dklist->xnu_klist, event->events);
			libsimple_lock_unlock(&dklist->lock);
		}
	}

	return THREAD_AWAKENED;
};

int kevent_legacy_get_deadline(int flags, user_addr_t utimeout, uint64_t *deadline) {
	// TODO
	panic("kevent_legacy_get_deadline");
};

void dserver_kqueue_close(int fd) {

};

void dserver_kqueue_wakeup(struct kqueue* kq) {
	// kqueue lock is held here (and we do not drop it)
	dserver_kqueue_t* dkqueue = dserver_kqueue_for_xnu_kqueue(kq);

	if (kq->kq_state & KQ_PROCESSING) {
		// if we're already processing, there's no need to wake anything up
		return;
	}

	int ret = __linux_eventfd_write(dkqueue->epoll_fd, 1);
	if (ret < 0) {
		panic("Failed to write to wakeup fd");
	}
};

void dserver_knote_wakeup(struct knote* kn) {

};

void dserver_kqueue_process_wakeup(struct kqueue* kq) {

};
