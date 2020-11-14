/// @file
/// @brief Mutex
/// @details
/// This file defines the #TfsMutex type, a thin
/// wrapper over the pthread rwlock.

#ifndef TFS_MUTEX_H
#define TFS_MUTEX_H

// Imports
#include <pthread.h> // pthread

/// @brief Mutex lock
/// @details
/// This lock is a thin wrapper over the pthread
/// rw lock.
typedef struct TfsMutex {
	/// @brief Underlying mutex.
	pthread_mutex_t mutex;
} TfsMutex;

/// @brief Creates a new rw lock
TfsMutex tfs_mutex_new(void);

/// @brief Destroys an rw lock
void tfs_mutex_destroy(TfsMutex* self);

/// @brief Locks this rw lock
void tfs_mutex_lock(TfsMutex* self);

/// @brief Unlocks this rw lock
void tfs_mutex_unlock(TfsMutex* self);

#endif
