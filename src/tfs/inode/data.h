/// @file
/// Inode data variants.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <tfs/inode/dir.h>	// TfsInodeDir
#include <tfs/inode/file.h> // TfsInodeFile
#include <tfs/inode/type.h> // TfsInodeType

/// @brief Inode data
/// @details If the tag is `TfsInodeType::None`, then
///          all elements of this union are uninitialized.
typedef union TfsInodeData {
	/// @brief Data for `IfsNodeTypeFile`
	TfsInodeFile file;

	/// @brief Data for `IfsNodeTypeDir`
	TfsInodeDir dir;
} TfsInodeData;

#endif
