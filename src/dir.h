/// @file
/// Directory

#ifndef TFS_DIR_H
#define TFS_DIR_H

// Includes
#include <stdlib.h>	   // size_t
#include <inode_idx.h> // TfsInodeIdx

/// @brief Maximum file name length
enum
{
	MAX_FILE_NAME_LEN = 100
};

/// @brief Directory entry
typedef struct TfsDirEntry
{
	char name[MAX_FILE_NAME_LEN];
	TfsInodeIdx inode_idx;
} TfsDirEntry;

#endif
