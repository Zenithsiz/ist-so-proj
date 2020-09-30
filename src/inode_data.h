/// @file
/// Inode data variants.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <dir.h>		// TfsDirEntry
#include <inode_type.h> // TfsInodeType

/// @brief Data for `TfsInodeType::File`
typedef struct TfsInodeDataFile {
	/// @brief File contents
	char* contents;
} TfsInodeDataFile;

/// @brief Data for `TfsInodeType::Dir`
typedef struct TfsInodeDataDir {
	/// @brief Children
	TfsDirEntry* entries;
} TfsInodeDataDir;

/// @brief Inode data
/// @details If the tag is `TfsInodeType::None`, then
///          all elements of this union are uninitialized.
typedef union TfsInodeData {
	TfsInodeDataFile file;
	TfsInodeDataDir	 dir;
} TfsInodeData;

#endif
