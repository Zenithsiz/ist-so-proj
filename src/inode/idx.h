/// @file
/// Inode indexes

#ifndef TFS_INODE_IDX_H
#define TFS_INODE_IDX_H

// Includes
#include <stdlib.h> // size_t

/// @brief An inode index
typedef size_t TfsInodeIdx;

enum {
	/// @brief A nonexistant index
	TfsInodeIdxNone,
};

#endif
