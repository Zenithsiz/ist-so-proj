/// @file
/// @brief Inode directories.
/// @details
/// This file contains several types that define how directories
/// are defined and used within an inode.
///
/// Of special importance @ref TfsInodeDir is the data that is stored
/// in the inode itself.

#ifndef TFS_INODE_DIR_H
#define TFS_INODE_DIR_H

// Includes
#include <stdbool.h>	   // Bool
#include <stdio.h>		   // FILE
#include <stdlib.h>		   // size_t
#include <tfs/inode/idx.h> // TfsInodeIdx

enum {
	/// @brief Maximum file name length for each directory entry.
	TFS_DIR_MAX_FILE_NAME_LEN = 100,

	/// @brief Maximum number of entries for each directory entry.
	TFS_DIR_MAX_ENTRIES = 20,
};

/// @brief A directory entry.
/// @details
/// Each directory entry is specified by it's name and it's index in
/// the inode table.
typedef struct TfsDirEntry {
	/// @brief Name of the entry
	char name[TFS_DIR_MAX_FILE_NAME_LEN + 1];

	/// @brief Index of the entry on the inode table
	TfsInodeIdx inode_idx;
} TfsDirEntry;

/// @brief An inode directory.
/// @details
/// This inode is able to store several directory entries
/// to other inodes within the same table as it.
typedef struct TfsInodeDir {
	/// @brief All entries of this directory.
	TfsDirEntry* entries;
} TfsInodeDir;

/// @brief Result type for @ref tfs_inode_dir_add_entry
typedef struct TfsInodeDirAddEntryResult {
	/// @brief Result data
	enum {
		/// @brief Success
		TfsInodeDirAddEntryResultSuccess = 0,

		/// @brief Entry name was empty
		TfsInodeDirAddEntryResultErrorEmptyName = -1,

		/// @brief An entry with the same index already exists.
		TfsInodeDirAddEntryResultErrorDuplicateIdx = -2,

		/// @brief An entry with the same name already exists.
		TfsInodeDirAddEntryResultErrorDuplicateName = -3,

		/// @brief Directory was full
		TfsInodeDirAddEntryResultErrorFull = -4,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorDuplicateIdx`
		struct {
			/// @brief Name of the entry with the same index
			char* name;
		} duplicate_idx;

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

/// @brief Checks if a directory is empty.
/// @param dir The directory inode to check.
bool tfs_inode_dir_is_empty(TfsInodeDir* dir);

/// @brief Searches for an entry with a given name.
/// @param dir The directory inode to search in.
/// @param name Name of the entry to search for. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @param[out] idx Index of the entry, if found.
/// @return If the entry was found.
bool tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, size_t name_len, TfsInodeIdx* idx);

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
