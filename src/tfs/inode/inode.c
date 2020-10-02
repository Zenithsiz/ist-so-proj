#include "inode.h"

/// @brief Initialize an inode
void tfs_inode_init(TfsInode* inode, TfsInodeType type) {
	// Set the inode's type
	inode->type = type;

	switch (type) {
		// Set initial data to `NULL`
		case TfsInodeTypeFile: {
			inode->data.file.contents = NULL;
			break;
		}

		// Create children and set them all as empty
		case TfsInodeTypeDir: {
			inode->data.dir = tfs_inode_dir_new();
			break;
		}

		case TfsInodeTypeNone:
		default: {
			break;
		}
	}
}

/// @brief Drops an inode
void tfs_inode_drop(TfsInode* inode) {
	switch (inode->type) {
		// Deallocate a file's contents
		case TfsInodeTypeFile:
			free(inode->data.file.contents);
			break;

		// Deallocate a directory's children
		case TfsInodeTypeDir:
			tfs_inode_dir_drop(&inode->data.dir);
			break;

		case TfsInodeTypeNone:
		default:
			break;
	}
}
