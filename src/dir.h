/// @file
/// Directory

#ifndef TFS_DIR_H
#define TFS_DIR_H

// Includes
#include <inode_idx.h> // TfsInodeIdx
#include <stdlib.h>	   // size_t

enum {
	/// @brief Maximum file name length
	TFS_DIR_MAX_FILE_NAME_LEN = 100,

	/// @brief Maximum directory entries
	TFS_DIR_MAX_ENTRIES = 20,
};

/// @brief Directory entry
typedef struct TfsDirEntry {
	char		name[TFS_DIR_MAX_FILE_NAME_LEN];
	TfsInodeIdx inode_idx;
} TfsDirEntry;

#endif
