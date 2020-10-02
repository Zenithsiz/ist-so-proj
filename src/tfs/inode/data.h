/// @file
/// @brief Inode data
/// @details
/// This file contains the @ref TfsInodeData type, which
/// is responsible for holding any inode type.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <tfs/inode/dir.h>	// TfsInodeDir
#include <tfs/inode/file.h> // TfsInodeFile
#include <tfs/inode/type.h> // TfsInodeType

/// @brief Inode data
/// @details
/// This type contains all the possible data an inode may store.
/// If the inode has no data, i.e. it has type `None`, then none
/// of the variants are initialized.
typedef union TfsInodeData {
	/// @brief Data for `IfsNodeTypeFile`
	TfsInodeFile file;

	/// @brief Data for `IfsNodeTypeDir`
	TfsInodeDir dir;
} TfsInodeData;

#endif
