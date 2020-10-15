/// @file
/// @brief Inode indexes
/// @details
/// This file defines the #TfsInodeIdx type, used as an alias to
/// refer to an inode's index.
///
/// It also defines the #TFS_INODE_IDX_NONE value, which can be
/// used as a sentinel value to indicate an inode doesn't exist.
#ifndef TFS_INODE_IDX_H
#define TFS_INODE_IDX_H

// Includes
#include <stddef.h> // size_t

/// @brief An inode index
typedef size_t TfsInodeIdx;

/// @brief A nonexistant index
/// @details
/// This type is used as a sentinel value indicating
/// that an operation involving inodes failed.
#define TFS_INODE_IDX_NONE ((TfsInodeIdx)-1)

#endif
