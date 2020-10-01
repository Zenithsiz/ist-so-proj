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
/// @param max_inodes Max number of inodes in this table.
TfsInodeTable tfs_inode_table_new(size_t max_inodes);

/// @brief Drops an inode table
void tfs_inode_table_drop(TfsInodeTable* table);

/// @brief Creates a new inode in a table
///
/// @param table The table to create the inode in
/// @param type The type of inode to create
/// @param[out] idx Index of the created node
/// @param[out] data Data of the created node
TfsInodeTableCreateError tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type, TfsInodeIdx* idx, TfsInodeData** data);

/// @brief Removes an inode from the table
///
/// @param table The table to remove the inode from.
/// @param idx The index of the inode to delete.
TfsInodeTableRemoveError tfs_inode_table_remove(TfsInodeTable* table, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @param table The table to access the inode in
/// @param idx The index of the inode to access
/// @param[out] data Data of the inode
/// @param[out] type Type of the inode
TfsInodeTableGetError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints a tree of the whole inode table
///
/// @param table The table to print
/// @param out Print output
/// @param idx Inode to print (Used for recursing in directories).
/// @param name Current path
void tfs_inode_table_print_tree(TfsInodeTable* table, FILE* out, TfsInodeIdx idx, const char* name);

#endif
