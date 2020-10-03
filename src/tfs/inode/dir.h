/// @file
/// @brief Inode directories.
/// @details
/// This file contains 2 types that define how directories are defined
/// and used with inodes.
///
/// The @ref TfsDirEntry type is responsible for representing how each
/// entry in a directory is represented, while @ref TfsInodeDir is
/// responsible for holding the data that is stored into the inode itself
/// to manage the directory.

#ifndef TFS_INODE_DIR_H
#define TFS_INODE_DIR_H

// Includes
#include <stdbool.h>	   // Bool
#include <stdio.h>		   // FILE
#include <stdlib.h>		   // size_t
#include <tfs/inode/idx.h> // TfsInodeIdx

/// @brief Maximum file name length
/// @details
/// This does not include the null terminator.
#define TFS_DIR_MAX_FILE_NAME_LEN ((size_t)100)

/// @brief A directory entry.
/// @details
/// Each entry only stores it's name, and the inode it
/// represents. It is nothing more than a named 'link'
/// to the original inode.
typedef struct TfsDirEntry {
	/// @brief Name of the entry.
	char name[TFS_DIR_MAX_FILE_NAME_LEN + 1];

	/// @brief Underlying inode index.
	TfsInodeIdx inode_idx;
} TfsDirEntry;

/// @brief An inode directory
/// @details
/// Each directory is made up of multiple entries, each
/// with an unique name.
typedef struct TfsInodeDir {
	/// @brief All entries
	/// @details
	/// Heap pointer to all of the entries
	TfsDirEntry* entries;

	/// @brief Allocated entries
	/// @details
	/// The number of entries currently allocated.
	size_t capacity;
} TfsInodeDir;

/// @brief Result type for @ref tfs_inode_dir_add_entry
typedef struct TfsInodeDirAddEntryResult {
	/// @brief Result data
	enum {
		/// @brief Success
		TfsInodeDirAddEntryResultSuccess = 0,

		/// @brief Entry name was empty
		TfsInodeDirAddEntryResultErrorEmptyName = -1,

		/// @brief An entry with the same name already exists.
		TfsInodeDirAddEntryResultErrorDuplicateName = -3,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorDuplicateName`
		struct {
			/// @brief Index of the entry with the same name
			TfsInodeIdx idx;
		} duplicate_name;
	} data;
} TfsInodeDirAddEntryResult;

/// @brief Prints a textual representation of an result
/// @param result The result to print
/// @param out File descriptor to output to
void tfs_inode_dir_add_entry_result_print(const TfsInodeDirAddEntryResult* result, FILE* out);

/// @brief Creates a new inode directory
/// @details
/// The returned directory has no entries.
TfsInodeDir tfs_inode_dir_new(void);

/// @brief Drops an inode directory
void tfs_inode_dir_drop(TfsInodeDir* dir);

/// @brief Checks if a directory is empty.
/// @param dir The directory inode to check.
bool tfs_inode_dir_is_empty(const TfsInodeDir* dir);

/// @brief Searches for an entry with a given name.
/// @param dir The directory inode to search in.
/// @param name Name of the entry to search for. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @return The inode index if found or `TFS_INODE_IDX_NONE` otherwise.
TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* dir, const char* name, size_t name_len);

/// @brief Removes an entry given it's index.
/// @param dir The directory inode to remove the entry in.
/// @param idx The index of the entry to remove.
/// @return If successfully removed. If `false`, `idx` was not an entry within this directory.
bool tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx);

/// @brief Adds an entry given it's index and name.
/// @param dir The directory inode to add the entry in.
/// @param idx The index of the inode to add.
/// @param name The name of the entry to add. Is not required to be null terminated.
/// @param name_len Length of @p name.
TfsInodeDirAddEntryResult tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len);

#endif
