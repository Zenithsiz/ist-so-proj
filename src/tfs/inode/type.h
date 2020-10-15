/// @file
/// @brief Inode types
/// @details
/// This file defines the #TfsInodeType type, used
/// as the tag for all inodes.

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

/// @brief Returns a string representing @p self
const char* tfs_inode_type_str(TfsInodeType self);

#endif
