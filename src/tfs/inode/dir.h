/// @file
/// @brief Inode directories.
/// @details
/// This file defines the @ref TfsInodeDirEntry type, responsible for
/// holding the entry of a directory, and the @ref TfsInodeDir type,
/// responsible for holding a directory itself.

#ifndef TFS_INODE_DIR_H
#define TFS_INODE_DIR_H

// Includes
#include <stdbool.h>	   // bool
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
	/// @invariant
	/// All entries shall have different filenames,
	/// and none shall have an empty filename.
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
		/// @brief The entry name was empty
		TfsInodeDirAddEntryErrorEmptyName = -1,

		/// @brief An entry with the same filename already exists
		TfsInodeDirAddEntryErrorDuplicateName = -2,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for variant @ref TfsInodeDirAddEntryErrorDuplicateName
		struct {
			/// @brief Index of the entry with the same name
			TfsInodeIdx idx;

			/// @brief Directory index of the entry with the same name
			size_t dir_idx;
		} duplicate_name;
	} data;
} TfsInodeDirAddEntryError;

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_inode_dir_add_entry_error_print(const TfsInodeDirAddEntryError* self, FILE* out);

/// @brief Creates a new directory entry
/// @param idx Index of the new entry
/// @param name Name of the new entry
/// @param name_len Length of @p name
TfsInodeDirEntry tfs_inode_dir_entry_new(TfsInodeIdx idx, const char* name, size_t name_len);

/// @brief Destroys a directory entry
void tfs_inode_dir_entry_destroy(TfsInodeDirEntry* self);

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
/// @param[out] dir_idx Directory index of the inode found.
/// @return The inode index, if found, otherwise @ref TFS_INODE_IDX_NONE.
TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* self, const char* name, size_t name_len, size_t* dir_idx);

/// @brief Removes an entry given it's inode index.
/// @param self
/// @param idx The index of the entry to remove.
/// @return If successfully removed.
bool tfs_inode_dir_remove_entry(TfsInodeDir* self, TfsInodeIdx idx);

/// @brief Removes an entry given it's directory index
/// @param self
/// @param dir_idx The directory index of the entry to remove.
/// @return If successfully removed.
bool tfs_inode_dir_remove_entry_by_dir_idx(TfsInodeDir* self, size_t dir_idx);

/// @brief Adds an entry given it's index and name.
/// @param self
/// @param idx The index of the inode to add.
/// @param name The name of the entry to add. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @param[out] err Set is any errors occur
/// @return If successfully added the entry.
bool tfs_inode_dir_add_entry(TfsInodeDir* self, TfsInodeIdx idx, const char* name, size_t name_len, TfsInodeDirAddEntryError* err);

#endif
