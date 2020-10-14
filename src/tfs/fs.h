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
///
/// Although this type's methods aren't thread-safe, they provide
/// threading capabilities by signaling an external lock once all
/// relevant structures to the operation have been locked, and so
/// other methods may now be called, which will only operate after
/// the previous method completed if they depend on it, or else they
/// are run in parallel.
typedef struct TfsFs {
	/// @brief The inode table
	/// @invariant
	/// The first inode, with index 0, will always be
	/// a directory. This inode is also called the root.
	TfsInodeTable inode_table;
} TfsFs;

/// @brief Error type for @ref tfs_fs_find
typedef struct TfsFsFindError {
	/// @brief Result kind
	enum {
		/// @brief One of the parents was not a directory
		/// @details
		/// Given a path 'a/b/c', either 'a' or 'a/b'
		/// were not directories.
		TfsFsFindErrorParentsNotDir = -1,

		/// @brief Unable to find entry with name
		/// @details
		/// Given a path 'a/b/c', either 'b' didn't
		/// exist within 'a', or 'c' didn't exist within 'a/b'.
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
/// @param lock_kind Kind of lock to use for the file system
TfsFs tfs_fs_new(TfsLockKind lock_kind);

/// @brief Destroys a file system
/// @param self
void tfs_fs_destroy(TfsFs* self);

/// @brief Creates a new inode with path @p path
/// @param self
/// @param path The path of the inode to create
/// @param type The type of inode to create.
/// @param lock
/// Lock holding exclusive access to @p self .
/// Will be unlocked once this command has been
/// fully queued and the filesystem is ready for
/// more commands.
/// @param[out] err Set if any errors occur.
/// @return Index of the created inode. Or @ref TFS_INODE_IDX_NONE if an error occurred.
/// @details
/// The returned inode will be locked, and _must_ be unlocked.
TfsInodeIdx tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type, TfsLock* lock, TfsFsCreateError* err);

/// @brief Removes an inode with path @p path
/// @param self
/// @param path The path of the inode to remove
/// @param lock
/// Lock holding exclusive access to @p self .
/// Will be unlocked once this command has been
/// fully queued and the filesystem is ready for
/// more commands.
/// @param[out] err Set if any errors occur.
/// @return If successfully removed.
bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsLock* lock, TfsFsRemoveError* err);

/// @brief Locks and retrives an inode with path @p path for unique access.
/// @param self
/// @param path The path of the inode to get.
/// @param lock
/// Lock holding exclusive access to @p self .
/// Will be unlocked once this command has been
/// fully queued and the filesystem is ready for
/// more commands.
/// @param access Access type to lock te result with.
/// @param[out] err Set if any errors occur.
/// @return Index of the inode, if found. Otherwise @ref TFS_INODE_IDX_NONE
/// @warning The returned inode _must_ be unlocked.
TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsLock* lock, TfsLockAccess access, TfsFsFindError* err);

/// @brief Unlocks an inode index
/// @param self
/// @param idx The index to unlock
/// @return If successfully unlocked.
bool tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx);

/// @brief Accesses a locked inode.
/// @param self
/// @param idx The index of the inode to access.
/// @param[out] type The type of the inode.
/// @param[out] data The data of the inode.
/// @return If successfully found.
/// @warning The inode _must_ be locked either for shared or unique access.
bool tfs_fs_get_inode(TfsFs* self, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints the contents of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_fs_print(const TfsFs* self, FILE* out);

#endif
