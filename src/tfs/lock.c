#include "lock.h"

#include <assert.h> // assert

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
				.data = {.none = {.readers = 0, .writing = false}},
			};
		}
	}
}

void tfs_lock_destroy(TfsLock* self) {
	switch (self->kind) {
		case TfsLockKindMutex: {
			assert(pthread_mutex_destroy(&self->data.mutex) == 0);
			self->kind = TfsLockKindNone;
			break;
		}

		case TfsLockKindRWLock: {
			assert(pthread_rwlock_destroy(&self->data.rw_lock) == 0);
			self->kind = TfsLockKindNone;
			break;
		}

		default:
		case TfsLockKindNone: {
			assert(self->data.none.readers == 0 && !self->data.none.writing);
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
			assert(!self->data.none.writing);
			switch (access) {
				case TfsLockAccessShared: {
					self->data.none.readers++;
					break;
				}
				case TfsLockAccessUnique: {
					self->data.none.writing = true;
					break;
				}

				default: {
					break;
				}
			}
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
			if (self->data.none.readers != 0) {
				assert(!self->data.none.writing);
				self->data.none.readers--;
			}
			else {
				assert(self->data.none.writing);
				self->data.none.writing = false;
			}
			break;
		}
	}
}
