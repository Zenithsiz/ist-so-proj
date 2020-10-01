/// @file
/// Tecnico file system

#ifndef TFS_H
#define TFS_H

// Imports
#include <tfs/inode/table.h> // TfsInodeTable
#include <tfs/path.h>		 // TfsPath

/// @brief The file system
/// @invariant `inode_table` will always have a directory,
///            the 'root node', at the fist inode.
typedef struct TfsFileSystem {
	/// Inode table
	TfsInodeTable inode_table;
} TfsFileSystem;

/// @brief Error type for @ref tfs_create
typedef enum TfsFileSystemCreateError {
	/// @brief Success
	TfsFileSystemCreateErrorSuccess = 0,

	/// @brief No directory found for the given path's parent.
	TfsFileSystemCreateErrorInexistentParentDirectory = -1,

	/// @brief Parent was not a directory
	TfsFileSystemCreateErrorParentNotDir = -2,

	/// @brief Cannot add a file with the same name
	TfsFileSystemCreateErrorDuplicateName = -3,

	/// @brief Unable to create new inode
	TfsFileSystemCreateErrorCreateInode = -4,

	/// @brief Unable to add entry to directory
	TfsFileSystemInodeErrorAddEntry = -5,
} TfsFileSystemCreateError;

/// @brief Error type for @ref tfs_remove
typedef enum TfsFileSystemRemoveError {
	/// @brief Success
	TfsFileSystemRemoveErrorSuccess = 0,

	/// @brief No directory found for the given path's parent.
	TfsFileSystemRemoveErrorInexistentParentDirectory = -1,

	/// @brief Parent was not a directory
	TfsFileSystemRemoveErrorParentNotDir = -2,

	/// @brief Unable to find inode with filename
	TfsFileSystemRemoveErrorNameNotFound = -3,

	/// @brief Unable to remove non-empty directory
	TfsFileSystemRemoveErrorRemoveNonEmptyDir = -4,
} TfsFileSystemRemoveError;

/// @brief Error type for @ref tfs_find
typedef enum TfsFileSystemFindError {
	/// @brief Success
	TfsFileSystemFindErrorSuccess = 0,

	/// @brief One of the parents was not a directory
	TfsFileSystemFindErrorParentsNotDir = -1,

	/// @brief Unable to find inode with filename
	TfsFileSystemFindErrorNameNotFound = -2,
} TfsFileSystemFindError;

/// @brief Creates a new file system
/// @arg max_inodes The max number of inodes in this file system
TfsFileSystem tfs_new(size_t max_inodes);

/// @brief Drops a file system
void tfs_drop(TfsFileSystem* fs);

/// @brief Creates a new inode
TfsFileSystemCreateError tfs_create(TfsFileSystem* fs, TfsInodeType type, TfsPath path);

/// @brief Deletes an inode
TfsFileSystemRemoveError tfs_remove(TfsFileSystem* fs, TfsPath path);

/// @brief Returns the inode index of a path, if it exists
TfsFileSystemFindError tfs_find(TfsFileSystem* fs, TfsPath path, TfsInodeIdx* idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints the contents of this file system
/// @arg `out` File descriptor to output to.
void tfs_print(TfsFileSystem* fs, FILE* out);

#endif
