/// @file
/// @brief Rw lock
/// @details
/// This file defines the #TfsRwLock type, a thin
/// wrapper over the pthread rwlock.

#ifndef TFS_RW_LOCK_H
#define TFS_RW_LOCK_H

// Imports
#include <pthread.h> // pthread
#include <stdbool.h> // bool

/// @brief Lock access
typedef enum TfsRwLockAccess {
	/// @brief Shared access.
	/// @details
	/// This corresponds to a 'reader' lock.
	TfsRwLockAccessShared,

	/// @brief Unique access.
	/// @details
	/// This corresponds to a 'writer' lock.
	TfsRwLockAccessUnique,
} TfsRwLockAccess;

/// @brief Rw lock
/// @details
/// This lock is a thin wrapper over the pthread
/// rw lock.
typedef struct TfsRwLock {
	/// @brief Underlying rw lock.
	pthread_rwlock_t rw_lock;
} TfsRwLock;

/// @brief Creates a new rw lock
TfsRwLock tfs_rw_lock_new(void);

/// @brief Destroys an rw lock
void tfs_rw_lock_destroy(TfsRwLock* self);

/// @brief Locks this rw lock.
/// @param self
/// @param access Access type to lock the lock with
void tfs_rw_lock_lock(TfsRwLock* self, TfsRwLockAccess access);

/// @brief Attempts to lock this rw lock.
/// @param self
/// @param access Access type to lock the lock with
/// @return If successfully locked
bool tfs_rw_lock_try_lock(TfsRwLock* self, TfsRwLockAccess access);

/// @brief Unlocks this rw lock
void tfs_rw_lock_unlock(TfsRwLock* self);

#endif
