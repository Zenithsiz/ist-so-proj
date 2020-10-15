/// @file
/// @brief Thread synchronization locks.
/// @details
/// This file defines the @ref TfsLock type, used
/// as a general lock, capable of being several variants.

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
	/// @details
	/// This corresponds to a 'reader' lock.
	TfsLockAccessShared,

	/// @brief Unique access.
	/// @details
	/// This corresponds to a 'writer' lock.
	TfsLockAccessUnique,
} TfsLockAccess;

/// @brief Synchronization lock
/// @details
/// This lock is a tagged union of one of
/// the locks specified by @ref TfsLockKind .
///
/// With a lock type @ref TfsLockKindNone ,
/// this lock will verify all locks and unlocks
/// are correct, but as the lock may only be used
/// in a single-thread, it aborts if it would need
/// to wait for someone to release the lock.
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
