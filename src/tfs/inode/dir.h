/// @file
/// @brief Inode directories.
/// @details
/// This file contains several types that define how directories
/// are defined and used within an inode.
///
/// Of special importance @ref TfsInodeDir is the data that is stored
/// in the inode itself.

// Includes
#include <stdbool.h>	   // Bool
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

/// @brief Error type for all methods of @ref TfsInodeDir.
typedef enum TfsInodeDataDirError {
	/// @brief No error.
	TfsInodeDirErrorSuccess = 0,

	/// @brief Tried to add a duplicate entry.
	TfsInodeDataDirErrorDuplicateEntry = -1,

	/// @brief Unable to find entry with name.
	TfsInodeDataDirErrorNoNameMatch = -2,

	/// @brief Index was out of bounds.
	TfsInodeDataDirErrorIdxOutOfBounds = -3,

	/// @brief Index was not found in entries.
	TfsInodeDataDirErrorIdxNotFound = -4,

	/// @brief Name cannot be empty.
	TfsInodeDataDirErrorNameEmpty = -5,

	/// @brief Directory was full.
	TfsInodeDataDirErrorFull = -6,
} TfsInodeDataDirError;

/// @brief Checks if a directory is empty.
/// @param dir The directory inode to check.
bool tfs_inode_dir_is_empty(TfsInodeDir* dir);

/// @brief Searches for an entry with a given name.
/// @param dir The directory inode to search in.
/// @param name Name of the entry to search for. Is not required to be null terminated.
/// @param name_len Length of @p name.
/// @param[out] idx Index of the entry, if found.
TfsInodeDataDirError tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, size_t name_len, TfsInodeIdx* idx);

/// @brief Removes an entry given it's index.
/// @param dir The directory inode to remove the entry in.
/// @param idx The index of the entry to remove.
TfsInodeDataDirError tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx);

/// @brief Adds an entry given it's index and name.
/// @param dir The directory inode to add the entry in.
/// @param idx The index of the inode to add.
/// @param name The name of the entry to add. Is not required to be null terminated.
/// @param name_len Length of @p name.
TfsInodeDataDirError tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len);
