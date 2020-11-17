/// @file
/// @brief Inode tables
/// @details
/// This file defines the #TfsInodeTable type, responsible for
/// storing and managing all inodes of the file system.

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <stdbool.h>		 // bool
#include <stddef.h>			 // size_t
#include <stdio.h>			 // FILE
#include <tfs/inode/inode.h> // TfsInode
#include <tfs/rw_lock.h>	 // TfsRwLock

/// @brief An inode table
/// @details
/// Stores all inodes on the heap, expanding the storage
/// when required.
typedef struct TfsInodeTable {
	/// @brief All the inodes
	TfsInode* inodes;

	/// @brief Max number of inodes
	size_t capacity;
} TfsInodeTable;

typedef struct TfsLockedInode {
	/// Inode's index
	TfsInodeIdx idx;

	/// Inode's type
	TfsInodeType type;

	/// Inode's data
	TfsInodeData* data;
} TfsLockedInode;

/// @brief Creates a new, empty, inode table with capacity for @p size inodes.
TfsInodeTable tfs_inode_table_new(size_t size);

/// @brief Destroys the inode table
void tfs_inode_table_destroy(TfsInodeTable* self);

/// @brief Adds and locks an inode to this table for unique access.
/// @param self
/// @param type The type of inode to add. Must _not_ be `None`.
/// @return The index of the new inode, locked. Will _never_ be `None`.
TfsInodeIdx tfs_inode_table_add(TfsInodeTable* self, TfsInodeType type);

/// @brief Locks an inode and retrives it's data.
/// @param self
/// @param idx The index of the inode to lock. _Must_ be a valid inode index and not empty.
/// @param access Access type for the inode lock.
TfsLockedInode tfs_inode_table_lock(TfsInodeTable* self, TfsInodeIdx idx, TfsRwLockAccess access);

/// @brief Unlocks a locked inode.
/// @param self
/// @param idx The index of the inode to unlock. _Must_ be locked and non-empty.
void tfs_inode_table_unlock_inode(TfsInodeTable* self, TfsInodeIdx idx);

/// @brief Removes a locked inode
/// @param self
/// @param idx The index of the inode to delete. _Must_ be locked for unique access.
/// @details
/// This will also unlock the removed inode.
void tfs_inode_table_remove_inode(TfsInodeTable* self, TfsInodeIdx idx);

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
