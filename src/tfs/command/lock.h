/// @file
/// @brief Locking mechanism for @ref TfsCommandTable
/// @details
/// This file contains the @ref TfsCommandTableLock type,
/// which describes the locking mechanism for @ref TfsCommandTable.

#ifndef TFS_COMMAND_TABLE_LOCK_H
#define TFS_COMMAND_TABLE_LOCK_H

// Imports
#include <pthread.h> // pthread

/// @brief Lock kinds
typedef enum TfsCommandTableLockKind {
	/// @brief Mutex
	TfsCommandTableLockKindMutex,

	/// @brief Read-Write lock
	TfsCommandTableLockKindRWLock,

	/// @brief No lock
	TfsCommandTableLockKindNone,
} TfsCommandTableLockKind;

typedef struct TfsCommandTableLock {
	/// @brief Lock kind
	TfsCommandTableLockKind kind;

	/// @brief Lock data
	union {
		/// @brief Mutex for `Mutex` kind.
		pthread_mutex_t mutex;

		/// @brief RWLock for `RWLock` kind.
		pthread_rwlock_t rw_lock;
	} data;
} TfsCommandTableLock;

/// @brief Creates a new lock
TfsCommandTableLock tfs_command_table_lock_new(TfsCommandTableLockKind kind);

/// @brief Destroys a lock
void tfs_command_table_lock_destroy(TfsCommandTableLock* self);

/// @brief Locks this lock for reading
void tfs_command_table_lock_read_lock(TfsCommandTableLock* self);

/// @brief Locks this lock for writing
void tfs_command_table_lock_write_lock(TfsCommandTableLock* self);

/// @brief Unlocks this lock
void tfs_command_table_lock_unlock(TfsCommandTableLock* self);

#endif
