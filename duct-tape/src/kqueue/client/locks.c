#include <kern/locks.h>
#include <libsimple/lock.h>

void lck_mtx_init(lck_mtx_t* lck, lck_grp_t* grp, lck_attr_t* attr) {
	libsimple_lock_init(&lck->dtape_lock);
};

void lck_mtx_lock(lck_mtx_t* lck) {
	libsimple_lock_lock(&lck->dtape_lock);
};

void lck_mtx_unlock(lck_mtx_t* lck) {
	libsimple_lock_unlock(&lck->dtape_lock);
};
