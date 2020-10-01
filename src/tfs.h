/// @file
/// Tecnico file system

// Imports
#include <inode/table.h> // TfsInodeTable
#include <path.h>		 // TfsPath

/// @brief The file system
/// @invariant `inode_table` will always have a directory,
///            the 'root node', at the fist inode.
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
TfsFileSystemError tfs_create_inode(TfsFileSystem* fs, TfsInodeType type, TfsPath path);

/// @brief Deletes an inode
TfsFileSystemError tfs_delete_inode(TfsFileSystem* fs, TfsPath path);

/// @brief Returns the inode index of a path, if it exists
TfsFileSystemError tfs_find(TfsFileSystem* fs, TfsPath path, TfsInodeIdx* idx);

/// @brief Prints the contents of this file system
/// @arg `out` File descriptor to output to.
void tfs_print(TfsFileSystem* fs, FILE* out);
