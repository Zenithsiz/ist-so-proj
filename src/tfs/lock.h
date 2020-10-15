/// @file
/// @brief Locking mechanisms for this library
/// @details
/// This file contains the @ref TfsLock type,
/// which describes the locking mechanisms used
/// by the whole library

#ifndef TFS_LOCK_H
#define TFS_LOCK_H

// Imports
#include <pthread.h> // pthread
#include <stdbool.h> // bool
#include <stddef.h>	 // size_t

/// @brief Lock kinds
typedef enum TfsLockKind {
	/// @brief Mutex
	TfsLockKindMutex,

	/// @brief Read-Write lock
	TfsLockKindRWLock,

	/// @brief No lock
	TfsLockKindNone,
} TfsLockKind;

/// @brief Lock access
typedef enum TfsLockAccess {
	/// @brief Shared access.
	TfsLockAccessShared,

	/// @brief Unique access.
	TfsLockAccessUnique,
} TfsLockAccess;

/// @brief Synchronization lock
typedef struct TfsLock {
	/// @brief Lock kind
	TfsLockKind kind;

	/// @brief Lock data
	union {
		/// @brief Mutex for `Mutex` kind.
		pthread_mutex_t mutex;

		/// @brief RWLock for `RWLock` kind.
		pthread_rwlock_t rw_lock;

		/// @brief Lock for `None` kind
		struct {
			/// @brief Number of readers
			size_t readers;

			/// @brief If locked for writing
			bool writing;
		} none;
	} data;
} TfsLock;

/// @brief Creates a new lock
TfsLock tfs_lock_new(TfsLockKind kind);

/// @brief Destroys a lock
void tfs_lock_destroy(TfsLock* self);

/// @brief Locks this lock.
/// @param self
/// @param access Access type to lock the lock with
void tfs_lock_lock(TfsLock* self, TfsLockAccess access);

/// @brief Unlocks this lock
void tfs_lock_unlock(TfsLock* self);

#endif
