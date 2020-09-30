/// @file
/// Tecnico file system

// Imports
#include <inode/table.h> // TfsInodeTable

/// @brief The file system
typedef struct TfsFileSystem {
	/// Inode table
	TfsInodeTable inode_table;
} TfsFileSystem;

/// @brief Error type for all file system
typedef enum TfsFileSystemError {
	/// @brief Success
	TfsFileSystemErrorSuccess,

	/// @brief Unable to find file by path
	TfsFileSystemErrorNotFoundByPath,

	/// @brief Other
	// TODO: Remove
	TfsFileSystemErrorOther,

} TfsFileSystemError;

/// @brief Creates a new file system
/// @arg max_inodes The max number of inodes in this file system
TfsFileSystem tfs_new(size_t max_inodes);

/// @brief Drops a file system
void tfs_drop(TfsFileSystem* fs);

/// @brief Creates a new inode
TfsFileSystemError tfs_create_inode(TfsFileSystem* fs, TfsInodeType type, const char* path);

/// @brief Deletes an inode
TfsFileSystemError tfs_delete_inode(TfsFileSystem* fs, const char* path);

/// @brief Returns the inode index of a path, if it exists
TfsFileSystemError tfs_find(TfsFileSystem* fs, const char* path, TfsInodeIdx* idx);

/// @brief Prints the contents of this file system
/// @arg `out` File descriptor to output to.
void tfs_print(TfsFileSystem* fs, FILE* out);

/*
void init_fs(TfsInodeTable table);
void destroy_fs(TfsInodeTable table);
int	 is_dir_empty(TfsDirEntry* dirEntries);
int	 create(TfsInodeTable table, char* name, TfsInodeType nodeType);
int delete (TfsInodeTable table, char* name);

int	 lookup(TfsInodeTable table, char* name);
void print_tecnicofs_tree(TfsInodeTable table, FILE* fp);
*/
