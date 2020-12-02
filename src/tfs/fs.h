/// @file
/// @brief File system
/// @details
/// This file defines the #TfsFs type, responsible for
/// mantaining and operating the filesystem.

#ifndef TFS_FS_H
#define TFS_FS_H

// Imports
#include <stdio.h>			 // FILE*
#include <tfs/inode/table.h> // TfsInodeTable
#include <tfs/path.h>		 // TfsPath
#include <tfs/rw_lock.h>	 // TfsRwLock

/// @brief Root directory index
#define TFS_FS_ROOT_IDX ((TfsInodeIdx){.idx = 0})

/// @brief The file system
/// @details
/// As opposed to #TfsInodeTable , access to each inode
/// is made by it's path.
///
/// All methods of this type are _not_ thread-safe and must use
/// an external lock to synchronize. However, a capability is
/// offered by the filesystem to unlock the given lock once it
/// determines it is ready for another concurrent call, such
/// that the current call is completed as-if sequentially in
/// relation to any further calls.
typedef struct TfsFs {
	/// @brief The inode table
	/// @invariant
	/// The first inode, with index #TFS_FS_ROOT_IDX , will always be
	/// a directory. This inode is also called the root.
	TfsInodeTable inode_table;
} TfsFs;

/// @brief Error type for #tfs_fs_find
typedef struct TfsFsFindError {
	/// @brief Error kind
	enum {
		/// @brief One of the path's parents was not a directory
		/// @details
		/// Given a path 'a/b/c', either 'a' or 'a/b'
		/// were not directories.
		TfsFsFindErrorParentsNotDir,

		/// @brief One of the path's components did not exist
		/// @details
		/// Given a path 'a/b/c', either 'b' did not exist
		/// within 'a', or 'c' did not exist within 'a/b'.
		TfsFsFindErrorNameNotFound,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for variant #TfsFsFindErrorParentsNotDir
		struct {
			/// @brief Path of the entry that wasn't a directory
			TfsPath path;
		} parents_not_dir;

		/// @brief Data for variant #TfsFsFindErrorNameNotFound
		struct {
			/// @brief Path of the entry not found
			TfsPath path;
		} name_not_found;
	} data;
} TfsFsFindError;

/// @brief Result type for #tfs_fs_find
typedef struct TfsFsFindResult {
	/// @brief If the operation was successful
	bool success;

	/// @brief Result data
	union {
		/// @brief Locked inode
		TfsLockedInode inode;

		/// @brief Any possible errors
		TfsFsFindError err;
	} data;
} TfsFsFindResult;

/// @brief Error type for #tfs_fs_create
typedef struct TfsFsCreateError {
	/// @brief Error kind
	enum {
		/// @brief Unable to find the given path's parent directory.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not found.
		TfsFsCreateErrorInexistentParentDir,

		/// @brief Parent of the given path was not a directory.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not a directory.
		TfsFsCreateErrorParentNotDir,

		/// @brief Unable to add the new entry to the parent directory
		/// @details
		/// Given a path 'a/b/c', 'c' was not able to be added
		/// to 'a/b'.
		TfsFsCreateErrorAddEntry,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for variant #TfsFsCreateErrorInexistentParentDir
		struct {
			/// @brief Underlying error
			TfsFsFindError err;
		} inexistent_parent_dir;

		/// @brief Data for variant #TfsFsCreateErrorAddEntry
		struct {
			/// @brief Underlying error
			TfsInodeDirAddEntryError err;
		} add_entry;
	} data;
} TfsFsCreateError;

/// @brief Result type for #tfs_fs_create
typedef struct TfsFsCreateResult {
	/// @brief If the operation was successful
	bool success;

	/// @brief Result data
	union {
		/// @brief The inode's index
		TfsInodeIdx idx;

		/// @brief Any possible errors
		TfsFsCreateError err;
	} data;
} TfsFsCreateResult;

/// @brief Error type for #tfs_fs_remove
typedef struct TfsFsRemoveError {
	/// @brief Error kind
	enum {
		/// @brief Unable to find the given path's parent directory.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not found.
		TfsFsRemoveErrorInexistentParentDir,

		/// @brief Parent of the given path was not a directory.
		/// @details
		/// Given a path 'a/b/c', the path 'a/b' was not a directory.
		TfsFsRemoveErrorParentNotDir,

		/// @brief Unable to find given path
		/// @details
		/// Given a path 'a/b/c', the entry 'c' was not fond
		/// within the directory 'a/b'.
		TfsFsRemoveErrorNameNotFound,

		/// @brief The given path was a non-empty directory
		TfsFsRemoveErrorRemoveNonEmptyDir,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for variant TfsFsRemoveErrorInexistentParentDir
		struct {
			/// @brief Underlying error
			TfsFsFindError err;
		} inexistent_parent_dir;
	} data;
} TfsFsRemoveError;

/// @brief Result type for #tfs_fs_remove
typedef struct TfsFsRemoveResult {
	/// @brief If the operation was successful
	bool success;

	/// @brief Result data
	union {
		/// @brief Any possible errors
		TfsFsRemoveError err;
	} data;
} TfsFsRemoveResult;

/// @brief Error type for #tfs_fs_move
typedef struct TfsFsMoveError {
	/// @brief Error kind
	enum {
		/// @brief Unable to find the common ancestor
		/// @details
		/// Given the paths 'a/b/c1' and 'a/b/c2', the path 'a/b'
		/// was not found.
		TfsFsMoveErrorInexistentCommonAncestor,

		/// @brief The common ancestor was not a directory
		/// @details
		/// Given the paths 'a/b/c1' and 'a/b/c2', the path 'a/b'
		/// was not a directory
		TfsFsMoveErrorCommonAncestorNotDir,

		/// @brief The origin path was the destination's parent.
		/// @details
		/// Given the origin path 'a/b/c' and destination path 'a/b/c/d',
		/// the origin path was the destination's parent.
		TfsFsMoveErrorOriginDestinationParent,

		/// @brief The destination path was a parent of the origin.
		/// @details
		/// Given the origin path 'a/b/c/d' and destination path 'a/b/c',
		/// the destination path was an existing folder.
		TfsFsMoveErrorDestinationOriginParent,

		/// @brief Unable to find the origin path's parent directory.
		/// @details
		/// Given the origin path 'a/b/c', the path 'a/b' was not found.
		TfsFsMoveErrorInexistentOriginParentDir,

		/// @brief Unable to find the destination path's parent directory.
		/// @details
		/// Given the destination path, 'a/b/c', the path 'a/b' was not found.
		TfsFsMoveErrorInexistentDestinationParentDir,

		/// @brief The origin path's parent wasn't a directory
		/// @details
		/// Given a origin path 'a/b/c', the path 'a/b' was a directory
		TfsFsMoveErrorOriginParentNotDir,

		/// @brief The destination path's parent wasn't a directory
		/// @details
		/// Given a destination path 'a/b/c', the path 'a/b' was a directory
		TfsFsMoveErrorDestinationParentNotDir,

		/// @brief Unable to find the origin path's.
		/// @details
		/// Given the origin path 'a/b/c', the entry 'c' was not found in 'a/b'.
		TfsFsMoveErrorOriginNotFound,

		/// @brief Unable to add the new entry to the destination parent directory
		/// @details
		/// Given a destination path 'a/b/c', 'c' was not able to be added
		/// to 'a/b'.
		TfsFsMoveErrorAddEntry,

		/// @brief Unable to rename entry in parent directory
		/// @details
		/// Given a source path 'a/b/c' and destination path 'a/b/d', couldn't
		/// rename 'c' to 'd'.
		TfsFsMoveErrorRenameEntry,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for variant #TfsFsMoveErrorInexistentCommonAncestor
		struct {
			/// @brief Underlying error
			TfsFsFindError err;
		} inexistent_common_ancestor;

		/// @brief Data for variant #TfsFsMoveErrorAddEntry
		struct {
			/// @brief Underlying error
			TfsInodeDirAddEntryError err;
		} add_entry;

		/// @brief Data for variant #TfsFsMoveErrorRenameEntry
		struct {
			/// @brief Underlying error
			TfsInodeDirRenameError err;
		} rename_entry;
	} data;
} TfsFsMoveError;

/// @brief Result type for #tfs_fs_move
typedef struct TfsFsMoveResult {
	/// @brief If the operation was successful
	bool success;

	/// @brief Result data
	union {
		/// @brief The inode's
		TfsLockedInode inode;

		/// @brief Any possible errors
		TfsFsMoveError err;
	} data;
} TfsFsMoveResult;

/// @brief Error type for #tfs_fs_print
typedef struct TfsFsPrintError {
	/// @brief Error kind
	enum {
		/// @brief Unable to create file
		TfsFsPrintErrorCreate,
	} kind;
} TfsFsPrintError;

/// @brief Result type for #tfs_fs_remove
typedef struct TfsFsPrintResult {
	/// @brief If the operation was successful
	bool success;

	/// @brief Result data
	union {
		/// @brief Any possible errors
		TfsFsPrintError err;
	} data;
} TfsFsPrintResult;

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_find_error_print(const TfsFsFindError* self, FILE* out);

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_create_error_print(const TfsFsCreateError* self, FILE* out);

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_remove_error_print(const TfsFsRemoveError* self, FILE* out);

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_move_error_print(const TfsFsMoveError* self, FILE* out);

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_print_error_print(const TfsFsPrintError* self, FILE* out);

/// @brief Creates a new file system
TfsFs tfs_fs_new(void);

/// @brief Destroys a file system
void tfs_fs_destroy(TfsFs* self);

/// @brief Creates a new inode with path @p path
/// @param self
/// @param path The path of the inode to create
/// @param type The type of inode to create.
/// @details
/// The returned inode will be locked with unique access, and _must_ be unlocked.
TfsFsCreateResult tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type);

/// @brief Removes an inode with path @p path
/// @param self
/// @param path The path of the inode to remove
TfsFsRemoveResult tfs_fs_remove(TfsFs* self, TfsPath path);

/// @brief Locks and retrives an inode's data
/// @param self
/// @param path The path of the inode to get.
/// @param access Access type to lock the result with.
/// @warning The returned inode _must_ be unlocked.
TfsFsFindResult tfs_fs_find(TfsFs* self, TfsPath path, TfsRwLockAccess access);

/// @brief Moves an inode
/// @param self
/// @param orig_path The origin path to move from
/// @param dest_path The destination path to move to.
/// All parents of this path must exist.
/// @param access Access type to lock the source with.
/// @warning
/// The returned inode _must_ be unlocked.
TfsFsMoveResult tfs_fs_move(TfsFs* self, TfsPath orig_path, TfsPath dest_path, TfsRwLockAccess access);

/// @brief Prints the contents of the filesystem
/// @param self
/// @param file_name File to output to.
TfsFsPrintResult tfs_fs_print(TfsFs* self, const char* file_name);

/// @brief Unlocks an inode
/// @param self
/// @param idx The index of the inode to unlock. _Must_ be valid.
void tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx);

#endif
