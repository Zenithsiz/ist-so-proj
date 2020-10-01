/// @file
/// Inodes

#ifndef TFS_INODE_H
#define TFS_INODE_H

// Includes
#include <stdlib.h>			// size_t
#include <tfs/inode/data.h> // TfsInodeData
#include <tfs/inode/idx.h>	// TfsInodeIdx
#include <tfs/inode/type.h> // TfsInodeType

/// @brief An inode
/// @details The building block of the filesystem.
///          A tagged union, with tag `type`.
typedef struct TfsInode {
	/// @brief The type of this inode
	TfsInodeType type;

	/// @brief Data this inode holds.
	TfsInodeData data;
} TfsInode;

/// @brief Initialize an inode
void tfs_inode_init(TfsInode* inode, TfsInodeType type);

/// @brief Drops an inode
void tfs_inode_drop(TfsInode* inode);

#endif
