#ifndef _DARLINGSERVER_CLIENT_KQUEUE_H_
#define _DARLINGSERVER_CLIENT_KQUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct kevent;
struct kevent64_s;
struct timespec;

void dserver_kqueue_atfork_child(void);
int dserver_kqueue_create(void);
int dserver_kqueue_kevent64(int kq, const struct kevent64_s* changelist, int nchanges, struct kevent64_s* eventlist, int nevents, unsigned int flags, const struct timespec* timeout);
int dserver_kqueue_kevent(int	kq, const struct kevent* changelist, int nchanges, struct	kevent* eventlist, int nevents, const struct timespec* timeout);
void dserver_kqueue_close(int fd);

#ifdef __cplusplus
};
#endif

#endif // _DARLINGSERVER_CLIENT_KQUEUE_H_
