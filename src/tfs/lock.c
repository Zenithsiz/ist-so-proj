#include "lock.h"

TfsLock tfs_lock_new(TfsLockKind kind) {
	switch (kind) {
		case TfsKindMutex: {
			pthread_mutex_t mutex;
			pthread_mutex_init(&mutex, NULL);

			return (TfsLock){
				.kind = kind,
				.data = {.mutex = mutex},
			};
		}

		case TfsKindRWLock: {
			pthread_rwlock_t rw_lock;
			pthread_rwlock_init(&rw_lock, NULL);

			return (TfsLock){
				.kind = kind,
				.data = {.rw_lock = rw_lock},
			};
		}

		default:
		case TfsKindNone: {
			return (TfsLock){
				.kind = kind,
			};
		}
	}
}

void tfs_lock_destroy(TfsLock* self) {
	switch (self->kind) {
		case TfsKindMutex: {
			pthread_mutex_destroy(&self->data.mutex);
			break;
		}

		case TfsKindRWLock: {
			pthread_rwlock_destroy(&self->data.rw_lock);
			break;
		}

		default:
		case TfsKindNone: {
			break;
		}
	}
}

void tfs_lock_read_lock(TfsLock* self) {
	switch (self->kind) {
		case TfsKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsKindRWLock: {
			pthread_rwlock_rdlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsKindNone: {
			break;
		}
	}
}

void tfs_lock_write_lock(TfsLock* self) {
	switch (self->kind) {
		case TfsKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsKindRWLock: {
			pthread_rwlock_wrlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsKindNone: {
			break;
		}
	}
}

void tfs_lock_downgrade_lock(TfsLock* self) {
	switch (self->kind) {
		// Note: Mutexes can't be downgraded, they are already
		//       always exclusive.
		case TfsKindMutex: {
			break;
		}

		case TfsKindRWLock: {
			// Note: Locking a lock currently held as write
			//       to read will simply downgrade it.
			pthread_rwlock_rdlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsKindNone: {
			break;
		}
	}
}

void tfs_lock_unlock(TfsLock* self) {
	switch (self->kind) {
		case TfsKindMutex: {
			pthread_mutex_unlock(&self->data.mutex);
			break;
		}

		case TfsKindRWLock: {
			pthread_rwlock_unlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsKindNone: {
			break;
		}
	}
}
