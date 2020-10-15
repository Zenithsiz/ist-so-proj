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
#include <tfs/lock.h>		 // TfsLockKind
#include <tfs/path.h>		 // TfsPath

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
	/// The first inode, with index 0, will always be
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

/// @brief Creates a new file system
/// @param lock_kind Lock kind used by all inodes.
TfsFs tfs_fs_new(TfsLockKind lock_kind);

/// @brief Destroys a file system
void tfs_fs_destroy(TfsFs* self);

/// @brief Creates a new inode with path @p path
/// @param self
/// @param path The path of the inode to create
/// @param type The type of inode to create.
/// @param lock Lock to unlock once operation is atomic.
/// @param[out] err Set if any errors occur.
/// @return Index of the created inode. Or #TFS_INODE_IDX_NONE if an error occurred.
/// @details
/// The returned inode will be locked, and _must_ be unlocked.
TfsInodeIdx tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type, TfsLock* lock, TfsFsCreateError* err);

/// @brief Removes an inode with path @p path
/// @param self
/// @param path The path of the inode to remove
/// @param lock Lock to unlock once operation is atomic.
/// @param[out] err Set if any errors occur.
/// @return If successfully removed.
bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsLock* lock, TfsFsRemoveError* err);

/// @brief Locks and retrives an inode's data
/// @param self
/// @param path The path of the inode to get.
/// @param lock Lock to unlock once operation is atomic.
/// @param access Access type to lock te result with.
/// @param[out] type The type of the inode.
/// @param[out] data The data of the inode.
/// @param[out] err Set if any errors occur.
/// @return Index of the inode, if found. Otherwise #TFS_INODE_IDX_NONE
/// @warning The returned inode _must_ be unlocked.
/// @details
/// If @p access is `Unique`, it is guaranteed, once @p lock is released,
/// that no other calls to this function will lock the inode at @p path
/// before the caller unlocks the returned inode.
TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsLock* lock, TfsLockAccess access, TfsInodeType* type, TfsInodeData** data, TfsFsFindError* err);

/// @brief Unlocks an inode
/// @param self
/// @param idx The index of the inode to unlock
/// @return If successfully unlocked.
bool tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx);

/// @brief Prints the contents of the filesystem
/// @param self
/// @param out File to output to.
void tfs_fs_print(const TfsFs* self, FILE* out);

#endif
