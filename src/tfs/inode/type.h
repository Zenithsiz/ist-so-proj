/// @file
/// Inode types

#ifndef TFS_INODE_TYPE_H
#define TFS_INODE_TYPE_H

/// @brief Inode types
typedef enum TfsInodeType {
	/// @brief Empty inode
	TfsInodeTypeNone,

	/// @brief File inode
	TfsInodeTypeFile,

	/// @brief Directory inode
	TfsInodeTypeDir,
} TfsInodeType;

/// @brief Returns a string with the textual representation of `type`
/// @param type The inode type
const char* tfs_inode_type_str(TfsInodeType type);

#endif
