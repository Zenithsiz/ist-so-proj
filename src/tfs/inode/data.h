/// @file
/// @brief Inode data
/// @details
/// This file defines the #TfsInodeData type, responsible
/// for holding the data of each inode.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <tfs/inode/dir.h>	// TfsInodeDir
#include <tfs/inode/file.h> // TfsInodeFile

/// @brief Inode data
/// @details
/// An untagged union containing any data an inode may store.
typedef union TfsInodeData {
	/// @brief Data for #TfsInodeTypeFile
	TfsInodeFile file;

	/// @brief Data for #TfsInodeTypeDir
	TfsInodeDir dir;
} TfsInodeData;

#endif
