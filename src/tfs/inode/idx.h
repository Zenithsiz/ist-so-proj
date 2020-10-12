/// @file
/// @brief Inode indexes
/// @details
/// This file contains the @ref TfsInodeIdx type, which is a typedef
/// used to work with indices of inodes.
///
/// It also provides the @ref TFS_INODE_IDX_NONE value, which should be
/// used when an index does not exist.
#ifndef TFS_INODE_IDX_H
#define TFS_INODE_IDX_H

// Includes
#include <stddef.h> // size_t

/// @brief An inode index
typedef size_t TfsInodeIdx;

/// @brief A nonexistant index
/// @details
/// This type is useful, for example, in functions returning
/// an index that may fail.
#define TFS_INODE_IDX_NONE ((TfsInodeIdx)-1)

#endif
