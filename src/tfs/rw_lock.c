#include "rw_lock.h"

#include <assert.h> // assert

TfsRwLock tfs_rw_lock_new(void) {
	return (TfsRwLock){
		.rw_lock = PTHREAD_RWLOCK_INITIALIZER,
	};
}

void tfs_rw_lock_destroy(TfsRwLock* self) {
	assert(pthread_rwlock_destroy(&self->rw_lock) == 0);
}

void tfs_rw_lock_lock(TfsRwLock* self, TfsRwLockAccess access) {
	switch (access) {
		case TfsRwLockAccessShared: {
			assert(pthread_rwlock_rdlock(&self->rw_lock) == 0);
			break;
		}
		case TfsRwLockAccessUnique: {
			assert(pthread_rwlock_wrlock(&self->rw_lock) == 0);
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_rw_lock_unlock(TfsRwLock* self) {
	assert(pthread_rwlock_unlock(&self->rw_lock) == 0);
}
