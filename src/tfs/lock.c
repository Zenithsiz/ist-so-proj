#include "lock.h"

TfsLock tfs_lock_new(TfsLockKind kind) {
	switch (kind) {
		case TfsLockKindMutex: {
			return (TfsLock){
				.kind = kind,
				.data = {.mutex = PTHREAD_MUTEX_INITIALIZER},
			};
		}

		case TfsLockKindRWLock: {
			return (TfsLock){
				.kind = kind,
				.data = {.rw_lock = PTHREAD_RWLOCK_INITIALIZER},
			};
		}

		default:
		case TfsLockKindNone: {
			return (TfsLock){
				.kind = kind,
			};
		}
	}
}

void tfs_lock_destroy(TfsLock* self) {
	// Note: `pthread_*_destroy` can only fail if it's
	//       argument is not a valid mutex/rwlock.
	//       We ensure that those are correct and so the
	//       function call cannot fail.
	switch (self->kind) {
		case TfsLockKindMutex: {
			pthread_mutex_destroy(&self->data.mutex);
			break;
		}

		case TfsLockKindRWLock: {
			pthread_rwlock_destroy(&self->data.rw_lock);
			break;
		}

		default:
		case TfsLockKindNone: {
			break;
		}
	}
}

void tfs_lock_read_lock(TfsLock* self) {
	switch (self->kind) {
		case TfsLockKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsLockKindRWLock: {
			pthread_rwlock_rdlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsLockKindNone: {
			break;
		}
	}
}

void tfs_lock_write_lock(TfsLock* self) {
	switch (self->kind) {
		case TfsLockKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsLockKindRWLock: {
			pthread_rwlock_wrlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsLockKindNone: {
			break;
		}
	}
}

void tfs_lock_unlock(TfsLock* self) {
	switch (self->kind) {
		case TfsLockKindMutex: {
			pthread_mutex_unlock(&self->data.mutex);
			break;
		}

		case TfsLockKindRWLock: {
			pthread_rwlock_unlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsLockKindNone: {
			break;
		}
	}
}
