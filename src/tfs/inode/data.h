/// @file
/// Inode data variants.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <tfs/inode/dir.h>	// TfsInodeDir
#include <tfs/inode/type.h> // TfsInodeType

/// @brief Data for `TfsInodeType::File`
typedef struct TfsInodeFile {
	/// @brief File contents
	char* contents;
} TfsInodeFile;

/// @brief Inode data
/// @details If the tag is `TfsInodeType::None`, then
///          all elements of this union are uninitialized.
typedef union TfsInodeData {
	TfsInodeFile file;
	TfsInodeDir	 dir;
} TfsInodeData;

#endif
