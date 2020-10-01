/// @file
/// @brief File system
/// @details
/// This file contains the type @ref TfsFs, which is responsible
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
typedef struct TfsFs {
	/// @brief The inode table
	/// @invariant
	/// The first inode, called 'root node' will always
	/// be a directory.
	TfsInodeTable inode_table;
} TfsFs;

/// @brief Error type for @ref tfs_fs_create
typedef struct TfsFsCreateError {
	/// @brief Error kind
	enum {
		/// @brief Success
		TfsFsCreateErrorSuccess = 0,

		/// @brief No directory found for the given path's parent.
		/// @details
		/// Given a path 'a/b/c', the inode 'a/b' was not found.
		TfsFsCreateErrorInexistentParentDirectory = -1,

		/// @brief Parent was not a directory
		/// @details
		/// Given a path 'a/b/c', although 'a' was a directory,
		/// 'b', the parent of the entry to create, was not
		/// a directory.
		TfsFsCreateErrorParentNotDir = -2,

		/// @brief Cannot add a file with the same name
		/// @details
		/// Given a path 'a/b/c', an entry with the name
		/// 'c' already existed in 'a/b'.
		TfsFsCreateErrorDuplicateName = -3,

		/// @brief Unable to create new inode
		/// @details
		/// This is an inode table specific error,
		/// see the underlying `create_inode` data.
		TfsFsCreateErrorCreateInode = -4,

		/// @brief Unable to add entry to directory
		/// @details
		/// This is an inode directory specific error,
		/// see the underlying `add_entry` data.
		TfsFsCreateErrorAddEntry = -5,
	} kind;

	/// @brief Error data
	union {
		/// @brief Underlying error for `CreateInode` error.
		TfsInodeTableCreateError create_inode;

		/// @brief Underlying error for `AddEntry` error.
		TfsInodeDirError add_entry;
	} data;
} TfsFsCreateError;

/// @brief Error type for @ref tfs_fs_remove
typedef enum TfsFsRemoveError {
	/// @brief Success
	TfsFsRemoveErrorSuccess = 0,

	/// @brief No directory found for the given path's parent.
	TfsFsRemoveErrorInexistentParentDirectory = -1,

	/// @brief Parent was not a directory
	TfsFsRemoveErrorParentNotDir = -2,

	/// @brief Unable to find inode with filename
	TfsFsRemoveErrorNameNotFound = -3,

	/// @brief Unable to remove non-empty directory
	TfsFsRemoveErrorRemoveNonEmptyDir = -4,
} TfsFsRemoveError;

/// @brief Error type for @ref tfs_fs_find
typedef enum TfsFsFindError {
	/// @brief Success
	TfsFsFindErrorSuccess = 0,

	/// @brief One of the parents was not a directory
	TfsFsFindErrorParentsNotDir = -1,

	/// @brief Unable to find inode with filename
	TfsFsFindErrorNameNotFound = -2,
} TfsFsFindError;

/// @brief Creates a new file system
/// @details
/// This will allocate an inode table and use it
/// for the file system.
/// @param max_inodes The max number of inodes in this file system
TfsFs tfs_fs_new(size_t max_inodes);

/// @brief Drops a file system
/// @param fs Filesystem to drop
void tfs_fs_drop(TfsFs* fs);

/// @brief Creates a new inode
/// @param fs Filesystem to create in.
/// @param type Type of inode to create.
/// @param path Path of inode to to create.
TfsFsCreateError tfs_fs_create(TfsFs* fs, TfsInodeType type, TfsPath path);

/// @brief Deletes an inode.
/// @param fs Filesystem to remove from.
/// @param path Path of inode to remove.
TfsFsRemoveError tfs_fs_remove(TfsFs* fs, TfsPath path);

/// @brief Returns the inode index of a path, if it exists.
/// @param fs Filesystem to create in.
/// @param path Path of inode to to find.
/// @param[out] idx Index of the inode.
/// @param[out] type Type of the inode.
/// @param[out] data Data of the inode.
TfsFsFindError tfs_fs_find(TfsFs* fs, TfsPath path, TfsInodeIdx* idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints the contents of this file system
/// @param fs File system to print.
/// @param out File descriptor to output to.
void tfs_fs_print(TfsFs* fs, FILE* out);

#endif
