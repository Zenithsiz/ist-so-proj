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

#endif
