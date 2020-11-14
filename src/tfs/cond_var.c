#include "cond_var.h"

// Imports
#include <assert.h>

TfsCondVar tfs_cond_var_new(void) {
	return (TfsCondVar){
		.cond = PTHREAD_COND_INITIALIZER,
	};
}

void tfs_cond_var_destroy(TfsCondVar* self) {
	assert(pthread_cond_destroy(&self->cond) == 0);
}

void tfs_cond_var_wait(TfsCondVar* self, TfsMutex* mutex) {
	assert(pthread_cond_wait(&self->cond, &mutex->mutex) == 0);
}

void tfs_cond_var_signal(TfsCondVar* self) {
	assert(pthread_cond_signal(&self->cond) == 0);
}

void tfs_cond_var_broadcast(TfsCondVar* self) {
	assert(pthread_cond_broadcast(&self->cond) == 0);
}
