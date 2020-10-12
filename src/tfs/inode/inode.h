/// @file
/// @brief Inodes
/// @details
/// This file contains the @ref TfsInode type, the building block
/// for the file system.

#ifndef TFS_INODE_INODE_H
#define TFS_INODE_INODE_H

// Includes
#include <tfs/inode/data.h> // TfsInodeData
#include <tfs/inode/type.h> // TfsInodeType
#include <tfs/lock.h>		// TfsLock

/// @brief An inode
/// @details
/// Each inode is a tagged union, containing a
/// variant of @ref TfsInodeType, where `type`
/// is the tag and `data` is the data associated.
typedef struct TfsInode {
	/// @brief The type of this inode
	TfsInodeType type;

	/// @brief Data this inode holds.
	TfsInodeData data;

	/// @brief Lock for synchronizing this inode
	TfsLock lock;
} TfsInode;

/// @brief Creates a new inode
/// @param type The type of inode to create
/// @param lock_kind The kind of lock to use for this inode
TfsInode tfs_inode_new(TfsInodeType type, TfsLockKind lock_kind);

/// @brief Destroys an inode
void tfs_inode_destroy(TfsInode* self);

/// @brief Sets an inode to be empty
void tfs_inode_empty(TfsInode* self);

#endif
