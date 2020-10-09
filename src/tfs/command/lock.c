#include "lock.h"

TfsCommandTableLock tfs_command_table_lock_new(TfsCommandTableLockKind kind) {
	switch (kind) {
		case TfsCommandTableLockKindMutex: {
			pthread_mutex_t mutex;
			pthread_mutex_init(&mutex, NULL);

			return (TfsCommandTableLock){
				.kind = kind,
				.data = {.mutex = mutex},
			};
		}

		case TfsCommandTableLockKindRWLock: {
			pthread_rwlock_t rw_lock;
			pthread_rwlock_init(&rw_lock, NULL);

			return (TfsCommandTableLock){
				.kind = kind,
				.data = {.rw_lock = rw_lock},
			};
		}

		default:
		case TfsCommandTableLockKindNone: {
			return (TfsCommandTableLock){
				.kind = kind,
			};
		}
	}
}

void tfs_command_table_lock_destroy(TfsCommandTableLock* self) {
	switch (self->kind) {
		case TfsCommandTableLockKindMutex: {
			pthread_mutex_destroy(&self->data.mutex);
			break;
		}

		case TfsCommandTableLockKindRWLock: {
			pthread_rwlock_destroy(&self->data.rw_lock);
			break;
		}

		default:
		case TfsCommandTableLockKindNone: {
		}
	}
}

void tfs_command_table_lock_read_lock(TfsCommandTableLock* self) {
	switch (self->kind) {
		case TfsCommandTableLockKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsCommandTableLockKindRWLock: {
			pthread_rwlock_rdlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsCommandTableLockKindNone: {
		}
	}
}

void tfs_command_table_lock_write_lock(TfsCommandTableLock* self) {
	switch (self->kind) {
		case TfsCommandTableLockKindMutex: {
			pthread_mutex_lock(&self->data.mutex);
			break;
		}

		case TfsCommandTableLockKindRWLock: {
			pthread_rwlock_wrlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsCommandTableLockKindNone: {
		}
	}
}

void tfs_command_table_lock_unlock(TfsCommandTableLock* self) {
	switch (self->kind) {
		case TfsCommandTableLockKindMutex: {
			pthread_mutex_unlock(&self->data.mutex);
			break;
		}

		case TfsCommandTableLockKindRWLock: {
			pthread_rwlock_unlock(&self->data.rw_lock);
			break;
		}

		default:
		case TfsCommandTableLockKindNone: {
		}
	}
}
