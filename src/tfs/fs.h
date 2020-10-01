/// @file
/// @brief File system
/// @details
/// This file contains the type @ref TfsFs, which is responsible
/// for managing the whole file system, as well as enforcing certain invariants
/// on the inode table.

#ifndef TFS_H
#define TFS_H

// Imports
#include <stdio.h>			 // FILE*
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

/// @brief Result type for @ref tfs_fs_find
typedef struct TfsFsFindResult {
	/// @brief Result data
	enum {
		/// @brief Success
		TfsFsFindResultSuccess = 0,

		/// @brief One of the parents was not a directory
		TfsFsFindResultErrorParentsNotDir = -1,

		/// @brief Unable to find entry with base name
		TfsFsFindResultErrorNameNotFound = -2,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `Success`
		struct {
			/// @brief Inode index of the path
			TfsInodeIdx idx;

			/// @brief Inode type of the path
			TfsInodeType type;

			/// @brief Inode data of the path
			TfsInodeData* data;
		} success;

		/// @brief Data for `ErrorParentsNotDir`
		struct {
			/// @brief Path of the entry that wasn't a directory
			TfsPath path;
		} parents_not_dir;

		/// @brief Data for `ErrorNameNotFound`
		struct {
			/// @brief File name of the entry not found
			TfsPath entry_name;
		} name_not_found;
	} data;
} TfsFsFindResult;

/// @brief Result type for @ref tfs_fs_create
typedef struct TfsFsCreateResult {
	/// @brief Result kind
	enum {
		/// @brief Success
		TfsFsCreateResultSuccess = 0,

		/// @brief No directory found for the given path's parent.
		/// @details
		/// Given a path 'a/b/c', the inode 'a/b' was not found.
		TfsFsCreateResultErrorInexistentParentDir = -1,

		/// @brief Parent was not a directory
		/// @details
		/// Given a path 'a/b/c', although 'a' was a directory,
		/// 'b', the parent of the entry to create, was not
		/// a directory.
		TfsFsCreateResultErrorParentNotDir = -2,

		/// @brief Cannot add a file with the same name
		/// @details
		/// Given a path 'a/b/c', an entry with the name
		/// 'c' already existed in 'a/b'.
		TfsFsCreateResultErrorDuplicateName = -3,

		/// @brief Unable to create new inode
		/// @details
		/// This is an inode table specific error,
		/// see the underlying `create_inode` data.
		TfsFsCreateResultErrorCreateInode = -4,

		/// @brief Unable to add entry to directory
		/// @details
		/// This is an inode directory specific error,
		/// see the underlying `add_entry` data.
		TfsFsCreateResultErrorAddEntry = -5,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorInexistentParentDir`
		struct {
			/// @brief Underlying error
			TfsFsFindResult err;

			/// @brief Path of the parent
			TfsPath parent;
		} inexistent_parent_dir;

		/// @brief Data for `ErrorParentNotDir`.
		struct {
			/// @brief Path of the parent
			TfsPath parent;
		} parent_not_dir;

		/// @brief Data for `ErrorCreateInode`.
		struct {
			/// @brief Underlying error.
			TfsInodeTableCreateError err;
		} create_inode;

		/// @brief Data for `ErrorAddEntry`.
		struct {
			/// @brief Underlying error.
			TfsInodeDirError err;
		} add_entry;
	} data;
} TfsFsCreateResult;

/// @brief Result type for @ref tfs_fs_remove
typedef struct TfsFsRemoveResult {
	/// @brief Error kind
	enum {
		/// @brief Success
		TfsFsRemoveResultSuccess = 0,

		/// @brief No directory found for the given path's parent.
		TfsFsRemoveResultErrorInexistentParentDir = -1,

		/// @brief Parent was not a directory
		TfsFsRemoveResultErrorParentNotDir = -2,

		/// @brief Unable to find inode with filename
		TfsFsRemoveResultErrorNameNotFound = -3,

		/// @brief Unable to remove non-empty directory
		TfsFsRemoveResultErrorRemoveNonEmptyDir = -4,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorInexistentParentDir`
		struct {
			/// @brief Underlying error
			TfsFsFindResult err;

			/// @brief Path of the parent
			TfsPath parent;
		} inexistent_parent_dir;

		/// @brief Data for `ErrorParentNotDir`.
		struct {
			/// @brief Path of the parent
			TfsPath parent;
		} parent_not_dir;

		/// @brief Data for `ErrorNameNotFound`.
		struct {
			/// @brief Entry name
			TfsPath entry_name;
		} name_not_found;

		/// @brief Data for `ErrorRemoveNonEmptyDir`.
		struct {
			/// @brief Directory name
			TfsPath dir_name;
		} remove_non_empty_dir;
	} data;
} TfsFsRemoveResult;

/// @brief Prints a textual representation of an result
/// @param result The result to print
/// @param out File descriptor to output to
void tfs_fs_find_result_print(const TfsFsFindResult* result, FILE* out);

/// @brief Prints a textual representation of a result
/// @param result The result to print
/// @param out File descriptor to output to
void tfs_fs_create_result_print(const TfsFsCreateResult* result, FILE* out);

/// @brief Prints a textual representation of a result
/// @param result The result to print
/// @param out File descriptor to output to
void tfs_fs_remove_result_print(const TfsFsRemoveResult* result, FILE* out);

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
TfsFsCreateResult tfs_fs_create(TfsFs* fs, TfsInodeType type, TfsPath path);

/// @brief Deletes an inode.
/// @param fs Filesystem to remove from.
/// @param path Path of inode to remove.
TfsFsRemoveResult tfs_fs_remove(TfsFs* fs, TfsPath path);

/// @brief Returns the inode index of a path, if it exists.
/// @param fs Filesystem to create in.
/// @param path Path of inode to to find.
/// @param[out] idx Index of the inode.
/// @param[out] type Type of the inode.
/// @param[out] data Data of the inode.
TfsFsFindResult tfs_fs_find(TfsFs* fs, TfsPath path);

/// @brief Prints the contents of this file system
/// @param fs File system to print.
/// @param out File descriptor to output to.
void tfs_fs_print(TfsFs* fs, FILE* out);

#endif
