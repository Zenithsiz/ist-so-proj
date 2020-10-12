/// @file
/// @brief Inode directories.
/// @details
/// This file contains 2 types that define how directories are defined
/// and used with inodes.
///
/// The @ref TfsInodeDirEntry type is responsible for representing how each
/// entry in a directory is represented, while @ref TfsInodeDir is
/// responsible for holding the data that is stored into the inode itself
/// to manage the directory.

#ifndef TFS_INODE_DIR_H
#define TFS_INODE_DIR_H

// Includes
#include <stdbool.h>	   // Bool
#include <stddef.h>		   // size_t
#include <stdio.h>		   // FILE
#include <tfs/inode/idx.h> // TfsInodeIdx

/// @brief A directory entry.
/// @details
/// Each entry only stores it's name, and the inode it
/// represents. It is nothing more than a named 'link'
/// to the original inode.
typedef struct TfsInodeDirEntry {
	/// @brief Name of the entry.
	char* name;

	/// @brief Length of `name`
	size_t name_len;

	/// @brief Underlying inode index.
	TfsInodeIdx inode_idx;
} TfsInodeDirEntry;

/// @brief An inode directory
/// @details
/// Each directory is made up of multiple entries, each
/// with an unique name.
typedef struct TfsInodeDir {
	/// @brief All entries
	/// @details
	/// Heap pointer to all of the entries
	TfsInodeDirEntry* entries;

	/// @brief Allocated entries
	/// @details
	/// The number of entries currently allocated.
	size_t capacity;
} TfsInodeDir;

/// @brief Error type for @ref tfs_inode_dir_add_entry
typedef struct TfsInodeDirAddEntryError {
	/// @brief Error kind
	enum {
		/// @brief Entry name was empty
		TfsInodeDirAddEntryErrorEmptyName = -1,

		/// @brief An entry with the same name already exists.
		TfsInodeDirAddEntryErrorDuplicateName = -2,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `ErrorDuplicateName`
		struct {
			/// @brief Index of the entry with the same name
			TfsInodeIdx idx;
		} duplicate_name;
	} data;
} TfsInodeDirAddEntryError;

/// @brief Prints a textual representation of a result
/// @param self
/// @param out File descriptor to output to
void tfs_inode_dir_add_entry_error_print(const TfsInodeDirAddEntryError* self, FILE* out);

/// @brief Creates a new directory entry
/// @param idx Index of the new entry
/// @param name Name of the new entry
/// @param name_len Length of @p name
TfsInodeDirEntry tfs_inode_dir_entry_new(TfsInodeIdx idx, const char* name, size_t name_len);

/// @brief Destroys a directory entry
void tfs_inode_dir_entry_destroy(TfsInodeDirEntry* entry);

/// @brief Creates a new, empty, inode directory
TfsInodeDir tfs_inode_dir_new(void);

/// @brief Destroys an inode directory
void tfs_inode_dir_destroy(TfsInodeDir* self);

/// @brief Checks if a directory is empty.
bool tfs_inode_dir_is_empty(const TfsInodeDir* self);

/// @brief Searches for an entry with a given name.
/// @param self
/// @param name Name of the entry to search for. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @return The inode index if found. Otherwise @ref TFS_INODE_IDX_NONE.
TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* self, const char* name, size_t name_len);

/// @brief Removes an entry given it's index.
/// @param self
/// @param idx The index of the entry to remove.
/// @return If successfully removed. If `false`, `idx` was not an entry within this directory.
bool tfs_inode_dir_remove_entry(TfsInodeDir* self, TfsInodeIdx idx);

/// @brief Adds an entry given it's index and name.
/// @param self
/// @param idx The index of the inode to add.
/// @param name The name of the entry to add. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @param[out] err Set is any errors occur
/// @return If successfully added the entry.
bool tfs_inode_dir_add_entry(TfsInodeDir* self, TfsInodeIdx idx, const char* name, size_t name_len, TfsInodeDirAddEntryError* err);

#endif
