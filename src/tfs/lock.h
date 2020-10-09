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

/// @brief Lock kinds
typedef enum TfsLockKind {
	/// @brief Mutex
	TfsKindMutex,

	/// @brief Read-Write lock
	TfsKindRWLock,

	/// @brief No lock
	TfsKindNone,
} TfsLockKind;

typedef struct TfsLock {
	/// @brief Lock kind
	TfsLockKind kind;

	/// @brief Lock data
	union {
		/// @brief Mutex for `Mutex` kind.
		pthread_mutex_t mutex;

		/// @brief RWLock for `RWLock` kind.
		pthread_rwlock_t rw_lock;
	} data;
} TfsLock;

/// @brief Creates a new lock
TfsLock tfs_lock_new(TfsLockKind kind);

/// @brief Destroys a lock
void tfs_lock_destroy(TfsLock* self);

/// @brief Locks this lock for reading
void tfs_lock_read_lock(TfsLock* self);

/// @brief Locks this lock for writing
void tfs_lock_write_lock(TfsLock* self);

/// @brief Unlocks this lock
void tfs_lock_unlock(TfsLock* self);

#endif
