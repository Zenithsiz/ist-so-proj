/// @file
/// Inode data variants.

#ifndef TFS_INODE_DATA_H
#define TFS_INODE_DATA_H

// Imports
#include <dir.h>		// TfsDirEntry
#include <inode/type.h> // TfsInodeType

/// @brief Data for `TfsInodeType::File`
typedef struct TfsInodeFile {
	/// @brief File contents
	char* contents;
} TfsInodeFile;

/// @brief Data for `TfsInodeType::Dir`
typedef struct TfsInodeDir {
	/// @brief Children
	TfsDirEntry* entries;
} TfsInodeDir;

/// @brief Error type for `TfsInodeDataDirError` operations
typedef enum TfsInodeDataDirError {
	/// @brief No error
	TfsInodeDataDirErrorSuccess,

	/// @brief Unable to find entry with name
	TfsInodeDataDirErrorNoNameMatch,
} TfsInodeDataDirError;

/// @brief Checks if a directory is empty
int tfs_inode_dir_is_empty(TfsInodeDir* dir);

/// @brief Searches for a node
TfsInodeDataDirError tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, TfsInodeIdx* idx);

/// @brief Inode data
/// @details If the tag is `TfsInodeType::None`, then
///          all elements of this union are uninitialized.
typedef union TfsInodeData {
	TfsInodeFile file;
	TfsInodeDir	 dir;
} TfsInodeData;

#endif
