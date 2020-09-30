#include <inode/inode.h>

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
			/* Initializes entry table */
			inode->data.dir.entries = malloc(sizeof(TfsDirEntry) * TFS_DIR_MAX_ENTRIES);

			for (int i = 0; i < TFS_DIR_MAX_ENTRIES; i++) {
				inode->data.dir.entries[i].inode_idx = TfsInodeIdxNone;
			}
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
			free(inode->data.dir.entries);
			break;

		case TfsInodeTypeNone:
		default:
			break;
	}
}
