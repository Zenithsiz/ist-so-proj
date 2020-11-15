/// @file
/// @brief Thread synchronization condition locks.
/// @details
/// This file defines the #TfsCondVar type, used
/// as a wrapper over condition locks

#ifndef TFS_COND_VAR_H
#define TFS_COND_VAR_H

// Imports
#include <pthread.h>   // pthread
#include <tfs/mutex.h> // TfsMutex

/// @brief Synchronization condition variable
/// @details
/// This is a simple wrapper over the posix condition variable.
typedef struct TfsCondVar {
	/// @brief Cond var
	pthread_cond_t cond;
} TfsCondVar;

/// @brief Creates a new condition variable
TfsCondVar tfs_cond_var_new(void);

/// @brief Destroys a condition variable
void tfs_cond_var_destroy(TfsCondVar* self);

/// @brief Waits on this cond var until a signal is emmitted.
/// @param self
/// @param mutex The mutex to wait on. _MUST_ be locked.
/// @note
/// It is possible for a spurious wake up to occur, so
/// this should be checked in a loop along with the wakeup
/// condition.
void tfs_cond_var_wait(TfsCondVar* self, TfsMutex* mutex);

/// @brief Signals to a single thread waiting on this cond var.
void tfs_cond_var_signal(TfsCondVar* self);

/// @brief Signals to all thread waiting on this cond var.
void tfs_cond_var_broadcast(TfsCondVar* self);

#endif
