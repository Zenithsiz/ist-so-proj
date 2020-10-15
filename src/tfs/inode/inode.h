/// @file
/// @brief Inodes
/// @details
/// This file defines the #TfsInode type, which fully describes
/// each inode within the file system.

#ifndef TFS_INODE_INODE_H
#define TFS_INODE_INODE_H

// Includes
#include <tfs/inode/data.h> // TfsInodeData
#include <tfs/inode/type.h> // TfsInodeType
#include <tfs/lock.h>		// TfsLock

/// @brief An inode
/// @details
/// Each inode is a tagged union, containing
/// one of the variants described in #TfsInodeType
/// Each inode also contains a lock to ensure
/// synchronization during operations
typedef struct TfsInode {
	/// @brief Type of inode
	TfsInodeType type;

	/// @brief Inode data
	TfsInodeData data;

	/// @brief Lock for this inode.
	TfsLock lock;
} TfsInode;

/// @brief Creates a new inode
/// @param type The type of inode to create
/// @param lock_kind The kind of lock to use for this inode
TfsInode tfs_inode_new(TfsInodeType type, TfsLockKind lock_kind);

/// @brief Destroys an inode
/// @details
/// This function will wait until @p self is unlocked, but
/// the inode must _not_ be locked after a call to this
/// function is executed.
void tfs_inode_destroy(TfsInode* self);

/// @brief Sets an inode to be empty
/// @warning @p self _must_ be locked for unique access.
void tfs_inode_empty(TfsInode* self);

#endif
