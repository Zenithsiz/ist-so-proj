#include "rw_lock.h"

// Imports
#include <assert.h> // assert
#include <errno.h>	// EBUSY
#include <stdlib.h> // exit, EXIT_FAILURE

TfsRwLock tfs_rw_lock_new(void) {
	return (TfsRwLock){
		.rw_lock = PTHREAD_RWLOCK_INITIALIZER,
	};
}

void tfs_rw_lock_destroy(TfsRwLock* self) { //
	assert(pthread_rwlock_destroy(&self->rw_lock) == 0);
}

void tfs_rw_lock_lock(TfsRwLock* self, TfsRwLockAccess access) {
	switch (access) {
		case TfsRwLockAccessShared: {
			int err = pthread_rwlock_rdlock(&self->rw_lock);
			assert(err == 0);
			break;
		}
		case TfsRwLockAccessUnique: {
			int err = pthread_rwlock_wrlock(&self->rw_lock);
			assert(err == 0);
			break;
		}

		default: {
			break;
		}
	}
}

bool tfs_rw_lock_try_lock(TfsRwLock* self, TfsRwLockAccess access) {
	switch (access) {
		case TfsRwLockAccessShared: {
			int err = pthread_rwlock_tryrdlock(&self->rw_lock);
			switch (err) {
				case 0: return true;
				case EBUSY: return false;
				default: {
					exit(EXIT_FAILURE);
					return false;
				}
			}
		}
		case TfsRwLockAccessUnique: {
			int err = pthread_rwlock_trywrlock(&self->rw_lock);
			switch (err) {
				case 0: return true;
				case EBUSY: return false;
				default: {
					exit(EXIT_FAILURE);
					return false;
				}
			}
		}
		default: {
			return false;
		}
	}
}

void tfs_rw_lock_unlock(TfsRwLock* self) { //
	assert(pthread_rwlock_unlock(&self->rw_lock) == 0);
}
