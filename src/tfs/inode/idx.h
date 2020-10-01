/// @file
/// @brief Inode indexes
/// @details
/// This file contains the @ref TfsInodeIdx type, which is a typedef
/// used to work with indices of inodes.
///
/// It also provides the @ref TfsInodeIdxNone value, which should be
/// used when an index does not exist.
#ifndef TFS_INODE_IDX_H
#define TFS_INODE_IDX_H

// Includes
#include <stdlib.h> // size_t

/// @brief An inode index
typedef size_t TfsInodeIdx;

enum {
	/// @brief A nonexistant index
	TfsInodeIdxNone = -1,
};

#endif
