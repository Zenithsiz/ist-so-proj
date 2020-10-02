/// @file
/// @brief File inodes

#ifndef TFS_INODE_FILE_H
#define TFS_INODE_FILE_H

/// @brief Data for `TfsInodeType::File`
typedef struct TfsInodeFile {
	/// @brief File contents
	char* contents;
} TfsInodeFile;

#endif
