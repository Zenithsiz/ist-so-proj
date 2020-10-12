#include "lock.h"

#include <assert.h> // assert

TfsLock tfs_lock_new(TfsLockKind kind) {
	switch (kind) {
		case TfsLockKindMutex: {
			// Create a recursive mutex
			// Note: We require recursive due to our algorithms
			//       that use rwlocks as their base.
			pthread_mutex_t mutex;
			pthread_mutexattr_t mutex_attr;
			assert(pthread_mutexattr_init(&mutex_attr) == 0);
			assert(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE) == 0);
			assert(pthread_mutex_init(&mutex, &mutex_attr) == 0);

			return (TfsLock){
				.kind = kind,
				.data = {.mutex = mutex},
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
	switch (self->kind) {
		case TfsLockKindMutex: {
			assert(pthread_mutex_destroy(&self->data.mutex) == 0);
			break;
		}

		case TfsLockKindRWLock: {
			assert(pthread_rwlock_destroy(&self->data.rw_lock) == 0);
			break;
		}

		default:
		case TfsLockKindNone: {
			break;
		}
	}
}

void tfs_lock_lock(TfsLock* self, TfsLockAccess access) {
	switch (self->kind) {
		case TfsLockKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsLockKindRWLock: {
			switch (access) {
				case TfsLockAccessShared: {
					pthread_rwlock_rdlock(&self->data.rw_lock);
					break;
				}
				case TfsLockAccessUnique: {
					pthread_rwlock_wrlock(&self->data.rw_lock);
					break;
				}

				default: {
					break;
				}
			}
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
