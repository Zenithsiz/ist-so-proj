/// @file
/// @brief Tecnico file system
/// @details
/// This file contains the type @ref TfsFileSystem, which is responsible
/// for managing the whole file system, as well as enforcing certain invariants
/// on the inode table.

#ifndef TFS_H
#define TFS_H

// Imports
#include <tfs/inode/table.h> // TfsInodeTable
#include <tfs/path.h>		 // TfsPath

/// @brief The file system
/// @details
/// This type is a small wrapper over the inode table that
/// allows using paths ( @ref TfsPath ) to modify the inodes,
/// as opposed to having to keep track of inode numbers.
typedef struct TfsFileSystem {
	/// @brief The inode table
	/// @invariant
	/// The first inode, called 'root node' will always
	/// be a directory.
	TfsInodeTable inode_table;
} TfsFileSystem;

/// @brief Error type for @ref tfs_create
typedef struct TfsFileSystemCreateError {
	/// @brief Error kind
	enum {
		/// @brief Success
		TfsFileSystemCreateErrorSuccess = 0,

		/// @brief No directory found for the given path's parent.
		/// @details
		/// Given a path 'a/b/c', the inode 'a/b' was not found.
		TfsFileSystemCreateErrorInexistentParentDirectory = -1,

		/// @brief Parent was not a directory
		/// @details
		/// Given a path 'a/b/c', although 'a' was a directory,
		/// 'b', the parent of the entry to create, was not
		/// a directory.
		TfsFileSystemCreateErrorParentNotDir = -2,

		/// @brief Cannot add a file with the same name
		/// @details
		/// Given a path 'a/b/c', an entry with the name
		/// 'c' already existed in 'a/b'.
		TfsFileSystemCreateErrorDuplicateName = -3,

		/// @brief Unable to create new inode
		/// @details
		/// This is an inode table specific error,
		/// see the underlying `create_inode` data.
		TfsFileSystemCreateErrorCreateInode = -4,

		/// @brief Unable to add entry to directory
		/// @details
		/// This is an inode directory specific error,
		/// see the underlying `add_entry` data.
		TfsFileSystemCreateErrorAddEntry = -5,
	} kind;

	/// @brief Error data
	union {
		/// @brief Underlying error for `CreateInode` error.
		TfsInodeTableCreateError create_inode;

		/// @brief Underlying error for `AddEntry` error.
		TfsInodeDirError add_entry;
	} data;
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
/// @details
/// This will allocate an inode table and use it
/// for the file system.
/// @param max_inodes The max number of inodes in this file system
TfsFileSystem tfs_new(size_t max_inodes);

/// @brief Drops a file system
/// @param fs Filesystem to drop
void tfs_drop(TfsFileSystem* fs);

/// @brief Creates a new inode
/// @param fs Filesystem to create in.
/// @param type Type of inode to create.
/// @param path Path of inode to to create.
TfsFileSystemCreateError tfs_create(TfsFileSystem* fs, TfsInodeType type, TfsPath path);

/// @brief Deletes an inode.
/// @param fs Filesystem to remove from.
/// @param path Path of inode to remove.
TfsFileSystemRemoveError tfs_remove(TfsFileSystem* fs, TfsPath path);

/// @brief Returns the inode index of a path, if it exists.
/// @param fs Filesystem to create in.
/// @param path Path of inode to to find.
/// @param[out] idx Index of the inode.
/// @param[out] type Type of the inode.
/// @param[out] data Data of the inode.
TfsFileSystemFindError tfs_find(TfsFileSystem* fs, TfsPath path, TfsInodeIdx* idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints the contents of this file system
/// @param fs File system to print.
/// @param out File descriptor to output to.
void tfs_print(TfsFileSystem* fs, FILE* out);

#endif
