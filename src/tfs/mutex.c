#include "mutex.h"

#include <assert.h> // assert

TfsMutex tfs_mutex_new(void) {
	return (TfsMutex){
		.mutex = PTHREAD_MUTEX_INITIALIZER,
	};
}

void tfs_mutex_destroy(TfsMutex* self) {
	assert(pthread_mutex_destroy(&self->mutex) == 0);
}

void tfs_mutex_lock(TfsMutex* self) {
	assert(pthread_mutex_lock(&self->mutex) == 0);
}

void tfs_mutex_unlock(TfsMutex* self) {
	assert(pthread_mutex_unlock(&self->mutex) == 0);
}
