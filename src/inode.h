/// @file
/// Inodes

#ifndef TFS_INODE_H
#define TFS_INODE_H

// Includes
#include <stdlib.h> // size_t
#include <dir.h>	// TfsDirEntry
#include <error.h>	// TfsError

/// @brief Inode types
typedef enum TfsInodeType
{
	/// @brief Empty inode
	TfsInodeTypeNone,

	/// @brief File inode
	TfsInodeTypeFile,

	/// @brief Directory inode
	TfsInodeTypeDir,
} TfsInodeType;

/// @brief Data for `TfsInodeType::File`
typedef struct TfsInodeDataFile
{
	/// @brief File contents
	char *contents;
} TfsInodeDataFile;

/// @brief Data for `TfsInodeType::Dir`
typedef struct TfsInodeDataDir
{
	/// @brief Children
	TfsDirEntry *children;
} TfsInodeDataDir;

/// @brief Inode data
/// @details If the tag is `TfsInodeType::None`, then
///          all elements of this union are uninitialized.
typedef union TfsInodeData
{
	TfsInodeDataFile file;
	TfsInodeDataDir dir;
} TfsInodeData;

/// @brief An inode
/// @details The building block of the filesystem.
///          A tagged union, with tag `type`.
typedef struct TfsInode
{
	TfsInodeType type;
	TfsInodeData data;
} TfsInode;

/// @brief An inode index
typedef size_t TfsInodeIdx;

/// @brief Initializes an inode table
///
/// @arg table The table to initialize
/// @arg len The length of `table`
void tfs_inode_table_init(TfsInode *table, size_t len);

/// @brief Drops an inode table
///
/// @arg table The table to drop
/// @arg len The length of `table`
void tfs_inode_table_drop(const TfsInode *table, size_t len);

/// @brief Creates a new inode in a table
///
/// @arg table The table to create the inode in
/// @arg type The type of inode to create
/// @arg idx Out parameter with the index
TfsError tfs_inode_create(TfsInode *table, TfsInodeType type, TfsInodeIdx *idx);

/// @brief Deletes an inode from the table
///
/// @arg table The table to delete the inode from
/// @arg idx The index of the inode to delete
TfsError tfs_inode_delete(TfsInode *table, TfsInodeIdx idx);

/// @brief Accesses an inode from the table
///
/// @arg table The table to access the inode in
/// @arg idx The index of the inode to access
/// @arg data Out parameter with the data in the inode
/// @arg type Out parameter with the type in the inode
TfsError tfs_inode_get(TfsInode *table, int idx, TfsInodeData *data, TfsInodeType *type);

/*
int inode_set_file(int inumber, char *fileContents, int len);
int dir_reset_entry(int inumber, int sub_inumber);
int dir_add_entry(int inumber, int sub_inumber, char *sub_name);
void inode_print_tree(FILE *fp, int inumber, char *name);
*/

#endif
