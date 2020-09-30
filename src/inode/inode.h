/// @file
/// Inodes

#ifndef TFS_INODE_H
#define TFS_INODE_H

// Includes
#include <inode/data.h> // TfsInodeData
#include <inode/idx.h>	// TfsInodeIdx
#include <inode/type.h> // TfsInodeType
#include <stdlib.h>		// size_t

/// @brief An inode
/// @details The building block of the filesystem.
///          A tagged union, with tag `type`.
typedef struct TfsInode {
	TfsInodeType type;
	TfsInodeData data;
} TfsInode;

/// @brief Initialize an inode
void tfs_inode_init(TfsInode* inode, TfsInodeType type);

/// @brief Drops an inode
void tfs_inode_drop(TfsInode* inode);

#endif
