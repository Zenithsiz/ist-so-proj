/// @file
/// @brief Inodes
/// @details
/// This file contains the @ref TfsInode type, the building block
/// for the file system.

#ifndef TFS_INODE_H
#define TFS_INODE_H

// Includes
#include <stdlib.h>			// size_t
#include <tfs/inode/data.h> // TfsInodeData
#include <tfs/inode/idx.h>	// TfsInodeIdx
#include <tfs/inode/type.h> // TfsInodeType

/// @brief An inode
/// @details
/// Each inode is a tagged union, containing a
/// variant of @ref TfsInodeType, where `type`
/// is the tag and `data` is the data associated.
// TODO: Support multiple hard links by keeping a counter.
typedef struct TfsInode {
	/// @brief The type of this inode
	TfsInodeType type;

	/// @brief Data this inode holds.
	TfsInodeData data;
} TfsInode;

/// @brief Creates a new inode
/// @param type The type of inode to create
TfsInode tfs_inode_new(TfsInodeType type);

/// @brief Drops an inode
/// @param inode The inode to drop.
void tfs_inode_drop(TfsInode* inode);

#endif
