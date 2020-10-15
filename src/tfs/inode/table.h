/// @file
/// @brief Inode tables
/// @details
/// This file defines the @ref TfsInodeTable type, responsible for
/// storing and managing all inodes of the file system.

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <stdbool.h>		 // bool
#include <stddef.h>			 // size_t
#include <stdio.h>			 // FILE
#include <tfs/inode/inode.h> // TfsInode
#include <tfs/lock.h>		 // TfsLock

/// @brief An inode table
/// @details
/// Stores all inodes on the heap, expanding the storage
/// when required.
typedef struct TfsInodeTable {
	/// @brief All the inodes
	TfsInode* inodes;

	/// @brief Allocated inodes
	size_t capacity;

	/// @brief Rwlock to lock during table growth
	/// @details
	/// It would also be possible for the implementation of
	/// `add` to lock all inodes with unique access, thus granting
	/// exclusive access to the whole table, but that approach does
	/// not scale well with the number of inodes, so an rwlock is
	/// used instead, locking for shared each time an inode is locked,
	/// unlocking when an inode is unlocked, and locking for unique
	/// access in `add`
	TfsLock rw_lock;

	/// @brief Lock kind used for all inodes
	TfsLockKind lock_kind;
} TfsInodeTable;

/// @brief Creates a new, empty, inode table
/// @param lock_kind Lock kind used by all inodes.
TfsInodeTable tfs_inode_table_new(TfsLockKind lock_kind);

/// @brief Destroys the inode table
void tfs_inode_table_destroy(TfsInodeTable* self);

/// @brief Adds and locks an inode to this table
/// @param self
/// @param type The type of inode to add
/// @param access The access to which to lock the inode with
/// @details
/// This is thread-safe and will wait for all locked inodes
/// to be unlocked before modifying the inode table.
/// The returned inode _must_ be unlocked.
TfsInodeIdx tfs_inode_table_add(TfsInodeTable* self, TfsInodeType type, TfsLockAccess access);

/// @brief Locks an inode and retrives it's data.
/// @param self
/// @param idx The index of the inode to lock.
/// @param access Access type to use for the lock.
/// @param[out] type The type of the inode.
/// @param[out] data The data of the inode.
/// @return If successfully locked.
/// @details
/// This function is thread-safe.
/// The returned inode _must_ be unlocked.
bool tfs_inode_table_lock(TfsInodeTable* self, TfsInodeIdx idx, TfsLockAccess access, TfsInodeType* type, TfsInodeData** data);

/// @brief Unlocks an inode.
/// @param self
/// @param idx The index of the inode to unlock
/// @return If successfully unlocked.
/// @warning The inode _must_ be locked for either shared or unique access.
/// @details
/// This function is thread-safe.
bool tfs_inode_table_unlock_inode(TfsInodeTable* self, TfsInodeIdx idx);

/// @brief Removes a locked inode
/// @param self
/// @param idx The index of the inode to delete
/// @return If successfully removed
/// @warning The inode _must_ be locked for unique access.
/// @details
/// This function is thread-safe.
/// This will also unlock the inode.
bool tfs_inode_table_remove_inode(TfsInodeTable* self, TfsInodeIdx idx);

/// @brief Prints an inode's path, along with all of it's children's.
/// @param self
/// @param idx The index of the inode to print.
/// @param out File to output to.
/// @param path Path of @p idx to print.
/// @details
/// This function is _not_ thread-safe.
/// All children's paths will be preprended with this inode's path, @p path
void tfs_inode_table_print_tree(const TfsInodeTable* self, TfsInodeIdx idx, FILE* out, const char* path);

#endif
