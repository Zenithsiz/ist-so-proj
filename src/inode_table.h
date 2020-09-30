/// @file
/// Inode tables

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <inode.h> // TfsInode
#include <stdio.h> // FILE

/// @brief An inode table
typedef struct TfsInodeTable {
	/// All of the inodes
	TfsInode* inodes;

	/// Number of inodes
	size_t len;
} TfsInodeTable;

/// @brief Error type for all inode table functions
typedef enum TfsInodeTableError {
	/// Inode table was full
	TfsInodeTableErrorSuccess,

	/// Inode table was full
	TfsInodeTableErrorInodeTableFull,

	/// Inode index was out of bounds
	TfsInodeTableErrorInodeIdxOutOfBounds,

	/// Inode sub-index was out of bounds
	TfsInodeTableErrorInodeSubIdxOutOfBounds,

	/// Inode index was not a directory
	TfsInodeTableErrorInodeIdxNotDir,

	/// Unable to find directory entry with sub index
	TfsInodeTableErrorSubIdxNotFoundInDirEntries,

	/// Directory entry name mustn't be empty
	TfsInodeTableErrorDirEntryNameEmpty,

	/// Inode directory was full
	TfsInodeTableErrorDirFull,
} TfsInodeTableError;

/// @brief Initializes an inode table
///
/// @arg table The table to initialize
/// @arg len The length of `table`
void tfs_inode_table_init(TfsInodeTable table);

/// @brief Drops an inode table
///
/// @arg table The table to drop
/// @arg len The length of `table`
void tfs_inode_table_drop(TfsInodeTable table);

/// @brief Creates a new inode in a table
///
/// @arg table The table to create the inode in
/// @arg type The type of inode to create
/// @arg idx Out parameter with the index
TfsInodeTableError
tfs_inode_table_create(TfsInodeTable table, TfsInodeType type, TfsInodeIdx* idx);

/// @brief Deletes an inode from the table
///
/// @arg table The table to delete the inode from
/// @arg idx The index of the inode to delete
TfsInodeTableError
tfs_inode_table_delete(TfsInodeTable table, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @arg table The table to access the inode in
/// @arg idx The index of the inode to access
/// @arg data Out parameter with the data in the inode
/// @arg type Out parameter with the type in the inode
TfsInodeTableError
tfs_inode_table_get(TfsInodeTable table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData* data);

TfsInodeTableError
tfs_inode_table_set_file(TfsInodeTable table, TfsInodeIdx inumber, char* fileContents, size_t len);
TfsInodeTableError
tfs_inode_table_dir_reset_entry(TfsInodeTable table, TfsInodeIdx inumber, TfsInodeIdx sub_inumber);
TfsInodeTableError
tfs_inode_table_dir_add_entry(TfsInodeTable table, TfsInodeIdx inumber, TfsInodeIdx sub_inumber, const char* sub_name);
TfsInodeTableError
tfs_inode_table_print_tree(TfsInodeTable table, FILE* fp, TfsInodeIdx inumber, const char* name);

#endif
