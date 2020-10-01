/// @file
/// Inode tables

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <stdio.h>			 // FILE
#include <tfs/inode/inode.h> // TfsInode

/// @brief An inode table
typedef struct TfsInodeTable {
	/// All of the inodes
	TfsInode* inodes;

	/// Number of inodes
	size_t len;
} TfsInodeTable;

/// @brief Error type for @ref tfs_inode_table_create
typedef enum TfsInodeTableCreateError {
	/// @brief Success
	TfsInodeTableCreateErrorSuccess,

	/// @brief Inode table was full
	TfsInodeTableCreateErrorFull,
} TfsInodeTableCreateError;

/// @brief Error type for @ref tfs_inode_table_remove
typedef enum TfsInodeTableRemoveError {
	/// @brief Success
	TfsInodeTableRemoveErrorSuccess,

	/// @brief No inode with `idx` was found
	TfsInodeTableRemoveErrorInvalidIdx,
} TfsInodeTableRemoveError;

/// @brief Error type for @ref tfs_inode_table_get
typedef enum TfsInodeTableGetError {
	/// @brief Success
	TfsInodeTableGetErrorSuccess,

	/// @brief No inode with `idx` was found
	TfsInodeTableGetErrorInvalidIdx,
} TfsInodeTableGetError;

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
TfsInodeTableCreateError tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type, TfsInodeIdx* idx, TfsInodeData** data);

/// @brief Removes an inode from the table
///
/// @arg table The table to delete the inode from
/// @arg idx The index of the inode to delete
TfsInodeTableRemoveError tfs_inode_table_remove(TfsInodeTable* table, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @arg table The table to access the inode in
/// @arg idx The index of the inode to access
/// @arg data Out parameter with the data in the inode
/// @arg type Out parameter with the type in the inode
TfsInodeTableGetError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data);

//TfsInodeTableError tfs_inode_table_set_file(TfsInodeTable* table, TfsInodeIdx inumber, char* fileContents, size_t len);

void tfs_inode_table_print_tree(TfsInodeTable* table, FILE* fp, TfsInodeIdx inumber, const char* name);

#endif
