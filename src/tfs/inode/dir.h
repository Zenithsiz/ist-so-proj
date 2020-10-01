/// @file
/// Directory inodes

// Includes
#include <stdlib.h>		   // size_t
#include <tfs/inode/idx.h> // TfsInodeIdx

enum {
	/// @brief Maximum file name length
	TFS_DIR_MAX_FILE_NAME_LEN = 100,

	/// @brief Maximum directory entries
	TFS_DIR_MAX_ENTRIES = 20,
};

/// @brief Directory entry
typedef struct TfsDirEntry {
	char		name[TFS_DIR_MAX_FILE_NAME_LEN];
	TfsInodeIdx inode_idx;
} TfsDirEntry;

/// @brief Data for `TfsInodeType::Dir`
typedef struct TfsInodeDir {
	/// @brief Children
	TfsDirEntry* entries;
} TfsInodeDir;

/// @brief Error type for `TfsInodeDataDirError` operations
typedef enum TfsInodeDataDirError {
	/// @brief No error
	TfsInodeDirErrorSuccess,

	/// @brief Unable to find entry with name
	TfsInodeDataDirErrorNoNameMatch,

	/// @brief Index was out of bounds
	TfsInodeDataDirErrorIdxOutOfBounds,

	/// @brief Index was not found in entries
	TfsInodeDataDirErrorIdxNotFound,

	/// @brief Name cannot be empty
	TfsInodeDataDirErrorNameEmpty,

	/// @brief Directory was full
	TfsInodeDataDirErrorFull,
} TfsInodeDataDirError;

/// @brief Checks if a directory is empty
int tfs_inode_dir_is_empty(TfsInodeDir* dir);

/// @brief Searches for a node
TfsInodeDataDirError tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, size_t name_len, TfsInodeIdx* idx);

TfsInodeDataDirError tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx);

TfsInodeDataDirError tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len);
