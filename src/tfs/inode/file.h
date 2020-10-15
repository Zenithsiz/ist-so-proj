/// @file
/// @brief File inodes
/// @details
/// This file defines the @ref TfsInodeFile type, responsible
/// for representing files within an inode.

#ifndef TFS_INODE_FILE_H
#define TFS_INODE_FILE_H

/// @brief A file inode
/// @details
/// Each file simply contains memory to it's
/// file contents, which are freed when the inode
/// is destroyed.
typedef struct TfsInodeFile {
	/// @brief File contents
	char* contents;
} TfsInodeFile;

#endif
