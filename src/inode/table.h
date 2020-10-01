/// @file
/// Inode tables

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <inode/inode.h> // TfsInode
#include <stdio.h>		 // FILE

/// @brief An inode table
typedef struct TfsInodeTable {
	/// All of the inodes
	TfsInode* inodes;

	/// Number of inodes
	size_t len;
} TfsInodeTable;

/// @brief Error type for all inode table functions
typedef enum TfsInodeTableError {
	/// @brief Success
	TfsInodeTableErrorSuccess,

	/// @brief Inode table was full
	TfsInodeTableErrorInodeTableFull,

	/// @brief Inode index was out of bounds
	TfsInodeTableErrorInodeIdxOutOfBounds,

	/// @brief Inode sub-index was out of bounds
	TfsInodeTableErrorInodeSubIdxOutOfBounds,

	/// @brief Inode index was not a directory
	TfsInodeTableErrorInodeIdxNotDir,

	/// @brief Unable to find directory entry with sub index
	TfsInodeTableErrorSubIdxNotFoundInDirEntries,

	/// @brief Directory entry name mustn't be empty
	TfsInodeTableErrorDirEntryNameEmpty,

	/// @brief Inode directory was full
	TfsInodeTableErrorDirFull,
} TfsInodeTableError;

/// @brief Creates a new inode table
/// @arg `max_inodes` Max number of inodes in this table
TfsInodeTable tfs_inode_table_new(size_t max_inodes);

/// @brief Drops an inode table
void tfs_inode_table_drop(TfsInodeTable* table);

/// @brief Creates a new inode in a table
///
/// @arg table The table to create the inode in
/// @arg type The type of inode to create
/// @arg idx Out parameter with the index
TfsInodeTableError
tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type, TfsInodeIdx* idx, TfsInodeData** data);

/// @brief Deletes an inode from the table
///
/// @arg table The table to delete the inode from
/// @arg idx The index of the inode to delete
TfsInodeTableError
tfs_inode_table_delete(TfsInodeTable* table, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @arg table The table to access the inode in
/// @arg idx The index of the inode to access
/// @arg data Out parameter with the data in the inode
/// @arg type Out parameter with the type in the inode
TfsInodeTableError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData* data);

//TfsInodeTableError tfs_inode_table_set_file(TfsInodeTable* table, TfsInodeIdx inumber, char* fileContents, size_t len);

/// @brief Removes the directory entry with index `sub_inumber`
TfsInodeTableError tfs_inode_table_print_tree(TfsInodeTable* table, FILE* fp, TfsInodeIdx inumber, const char* name);

#endif
