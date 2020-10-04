/// @file
/// @brief Inode tables
/// @details
/// This file contains the type @ref TfsInodeTable, responsible for managing
/// a table of nodes.

#ifndef TFS_INODE_TABLE_H
#define TFS_INODE_TABLE_H

// Includes
#include <stdbool.h>		 // bool
#include <stdio.h>			 // FILE
#include <tfs/inode/inode.h> // TfsInode

/// @brief An inode table
/// @details
/// Stores all the inodes on the heap and can
/// expand when necessary.
typedef struct TfsInodeTable {
	/// @brief All the inodes
	TfsInode* inodes;

	/// @brief Allocated inodes
	size_t capacity;
} TfsInodeTable;

/// @brief Creates a new inode table
/// @details
/// The returned table has no inodes.
TfsInodeTable tfs_inode_table_new(void);

/// @brief Destroys an inode table
void tfs_inode_table_destroy(TfsInodeTable* self);

/// @brief Creates a new inode in a table
///
/// @param self
/// @param type The type of inode to create
/// @param[out] idx Index of the created index
/// @param[out] data Data for the created index.
void tfs_inode_table_create(TfsInodeTable* self, TfsInodeType type, TfsInodeIdx* idx, TfsInodeData** data);

/// @brief Removes an inode from the table
///
/// @param self
/// @param idx The index of the inode to delete.
/// @return If the inode was deleted.
bool tfs_inode_table_remove(TfsInodeTable* self, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @param self
/// @param idx The index of the inode to access
/// @param[out] data Data of the inode
/// @param[out] type Type of the inode
/// @return If the inode was found.
bool tfs_inode_table_get(TfsInodeTable* self, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data);

/// @brief Prints a tree of the whole inode table
///
/// @param self
/// @param out Print output
/// @param idx Inode to print (Used for recursing in directories).
/// @param name Current path
void tfs_inode_table_print_tree(TfsInodeTable* self, FILE* out, TfsInodeIdx idx, const char* name);

#endif
