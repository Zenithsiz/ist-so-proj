/// @file
/// @brief File system
/// @details
/// This file contains the type @ref TfsFs, which is responsible
/// for managing the whole file system, as well as enforcing certain
/// invariants on the inode table.

#ifndef TFS_FS_H
#define TFS_FS_H

// Imports
#include <stdio.h>			 // FILE*
#include <tfs/inode/table.h> // TfsInodeTable
#include <tfs/lock.h>		 // TfsLockKind
#include <tfs/path.h>		 // TfsPath

/// @brief The file system
/// @details
/// This type is a wrapper over the inode table that allows
/// using @ref TfsPath paths to modify the inodes.
typedef struct TfsFs {
	/// @brief The inode table
	/// @invariant
	/// The first inode, with index 0, will always be
	/// a directory. This inode is also called the root.
	TfsInodeTable inode_table;
} TfsFs;

/// @brief Error type for @ref tfs_fs_find_shared and @ref tfs_fs_find_unique
typedef struct TfsFsFindError {
	/// @brief Result kind
	enum {
		/// @brief One of the parents was not a directory
		/// @details
		/// Given a path 'a/b/c', either 'a' or 'a/b'
		/// were not directories. The path that failed
		/// can be found in @ref TfsFsFindError.data.parent_not_dir.path
		TfsFsFindErrorParentsNotDir = -1,

		/// @brief Unable to find entry with name
		/// @details
		/// Given a path 'a/b/c', either 'b' didn't
		/// exist within 'a', or 'c' didn't exist within 'a/b'.
		/// The path that didn't exist can be found in
		/// @ref TfsFsFindError.data.name_not_found.path
		TfsFsFindErrorNameNotFound = -2,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorParentsNotDir`
		struct {
			/// @brief Path of the entry that wasn't a directory
			TfsPath path;
		} parents_not_dir;

		/// @brief Data for `ErrorNameNotFound`
		struct {
			/// @brief Path of the entry not found
			TfsPath path;
		} name_not_found;
	} data;
} TfsFsFindError;

/// @brief Error type for @ref tfs_fs_create
typedef struct TfsFsCreateError {
	/// @brief Error kind
	enum {
		/// @brief No directory found for the given path's parent.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not found.
		TfsFsCreateErrorInexistentParentDir = -1,

		/// @brief Parent was not a directory
		/// @details
		/// Given a path 'a/b/c', although 'a' was a directory,
		/// 'b', the parent of the entry to create, was not
		/// a directory.
		TfsFsCreateErrorParentNotDir = -2,

		/// @brief Unable to add entry to directory
		/// @details
		/// This is an inode directory specific error,
		/// see the underlying `add_entry` data.
		TfsFsCreateErrorAddEntry = -3,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for `InexistentParentDir`
		struct {
			/// @brief Underlying error
			TfsFsFindError err;

			/// @brief Path of the parent
			TfsPath parent;
		} inexistent_parent_dir;

		/// @brief Data for `ParentNotDir`.
		struct {
			/// @brief Path of the parent
			TfsPath parent;
		} parent_not_dir;

		/// @brief Data for `AddEntry`.
		struct {
			/// @brief Underlying error.
			TfsInodeDirAddEntryError err;
		} add_entry;
	} data;
} TfsFsCreateError;

/// @brief Error type for @ref tfs_fs_remove
typedef struct TfsFsRemoveError {
	/// @brief Error kind
	enum {
		/// @brief No directory found for the given path's parent.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not found.
		TfsFsRemoveErrorInexistentParentDir = -1,

		/// @brief Parent was not a directory
		/// @details
		/// Given a path 'a/b/c', although 'a' was a directory,
		/// 'b', the parent of the entry to create, was not
		/// a directory.
		TfsFsRemoveErrorParentNotDir = -2,

		/// @brief Unable to find inode with filename
		/// @details
		/// Given a path 'a/b/c', the directory 'a/b' did
		/// not contain any file with the name 'c'.
		TfsFsRemoveErrorNameNotFound = -3,

		/// @brief Unable to remove non-empty directory
		/// @details
		/// Given a path 'a/b', the directory 'a/b' was
		/// not empty.
		TfsFsRemoveErrorRemoveNonEmptyDir = -4,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for `InexistentParentDir`
		struct {
			/// @brief Underlying error
			TfsFsFindError err;

			/// @brief Path of the parent
			TfsPath parent;
		} inexistent_parent_dir;

		/// @brief Data for `ParentNotDir`.
		struct {
			/// @brief Path of the parent
			TfsPath parent;
		} parent_not_dir;

		/// @brief Data for `NameNotFound`.
		struct {
			/// @brief Entry name
			TfsPath entry_name;
		} name_not_found;
	} data;
} TfsFsRemoveError;

/// @brief Prints a textual representation of @param self to @param out
/// @param self
/// @param out File to output to.
void tfs_fs_find_error_print(const TfsFsFindError* self, FILE* out);

/// @brief Prints a textual representation of @param self to @param out
/// @param self
/// @param out File to output to.
void tfs_fs_create_error_print(const TfsFsCreateError* self, FILE* out);

/// @brief Prints a textual representation of @param self to @param out
/// @param self
/// @param out File to output to.
void tfs_fs_remove_error_print(const TfsFsRemoveError* self, FILE* out);

/// @brief Creates a new file system
/// @param lock_kind Kind of lock to use for the file system
TfsFs tfs_fs_new(TfsLockKind lock_kind);

/// @brief Destroys a file system
/// @param self
void tfs_fs_destroy(TfsFs* self);

/// @brief Creates a new inode with path @param path
/// @param self
/// @param path The path of the inode to create
/// @param type The type of inode to create.
/// @param lock Lock unlocked when @param self is
///             ready to execute more commands.
/// @param[out] err Set if any errors occur.
/// @return Index of the created inode. Or @ref TfsInodeIdxNone if an error occurred.
/// @warning This function is _not_ thread-safe, caller
///          must wait for either the function to return,
///          or for @param lock to be unlocked.
TfsInodeIdx tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type, TfsLock* lock, TfsFsCreateError* err);

/// @brief Removes an inode with path @param path
/// @param self
/// @param path The path of the inode to remove
/// @param lock Lock unlocked when @param self is
///             ready to execute more commands.
/// @param[out] err Set if any errors occur.
/// @return If successfully removed.
/// @warning This function is _not_ thread-safe, caller
///          must wait for either the function to return,
///          or for @param lock to be unlocked.
bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsLock* lock, TfsFsRemoveError* err);

/// @brief Locks and retrives an inode with path @param path for unique access.
/// @param self
/// @param path The path of the inode to get.
/// @param lock Lock unlocked when @param self is
///             ready to execute more commands.
/// @param access Access type to lock te result with.
/// @param[out] err Set if any errors occur.
/// @return Index of the inode, if found. Else @ref TfsInodeIdxNone
/// @details
/// The returned inode must be unlocked.
TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsLock* lock, TfsLockAccess access, TfsFsFindError* err);

/// @brief Prints the contents of @param self to @param out
/// @param self
/// @param out File to output to.
void tfs_fs_print(const TfsFs* self, FILE* out);

#endif
