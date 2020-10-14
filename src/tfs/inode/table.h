/// @file
/// @brief Inode tables
/// @details
/// This file contains the type @ref TfsInodeTable, responsible for managing
/// all nodes within a file system.

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
/// Stores all inodes on the heap, expanding when required.
/// Although all inodes have a separate lock, they will all share
/// the same lock kind as this table.
typedef struct TfsInodeTable {
	/// @brief All the inodes
	TfsInode* inodes;

	/// @brief Allocated inodes
	size_t capacity;

	/// @brief Rwlock to synchronize inode borrowing
	///        and table grown
	TfsLock rw_lock;

	/// @brief Lock kind used for all inodes
	TfsLockKind lock_kind;
} TfsInodeTable;

/// @brief Creates a new empty inode table
/// @param lock_kind Lock kind used for synchronizing this table
///                  and all inodes within it.
TfsInodeTable tfs_inode_table_new(TfsLockKind lock_kind);

/// @brief Destroys an inode table
void tfs_inode_table_destroy(TfsInodeTable* self);

/// @brief Adds an inode to this table, locking it with @p access
/// @param self
/// @param type The type of node to add
/// @return The index of the created inode.
/// @details
/// This operation invalidates all pointers to inodes and their data.
/// This function is thread-safe and will wait for all borrowed inodes
/// to be returned before moving the table.
/// The returned inode must be unlocked.
TfsInodeIdx tfs_inode_table_add(TfsInodeTable* self, TfsInodeType type, TfsLockAccess access);

/// @brief Locks an inode
/// @param self
/// @param idx The index of the inode to lock.
/// @param access Access type to use for the lock.
/// @return If successfully locked.
/// @details
/// This function is thread-safe.
bool tfs_inode_table_lock(TfsInodeTable* self, TfsInodeIdx idx, TfsLockAccess access);

/// @brief Unlocks an inode.
/// @param self
/// @param idx The index of the inode to unlock
/// @return If successfully unlocked.
/// @warning The inode _must_ be locked for either shared or unique access.
/// @details
/// This function is thread-safe.
bool tfs_inode_table_unlock_inode(TfsInodeTable* self, TfsInodeIdx idx);

/// @brief Accesses a locked inode.
/// @param self
/// @param idx The index of the inode to access.
/// @param[out] type The type of the inode.
/// @param[out] data The data of the inode.
/// @return If successfully found.
/// @warning The inode _must_ be locked either for shared or unique access.
/// @details
/// This function is thread-safe.
bool tfs_inode_table_get(TfsInodeTable* self, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data);

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
/// All children's paths will be preprended with this inode's path, @p path
/// This function is _not_ thread-safe, no other function may be called
/// while this function is executing.
void tfs_inode_table_print_tree(const TfsInodeTable* self, TfsInodeIdx idx, FILE* out, const char* path);

#endif
