#include "inode.h"

TfsInode tfs_inode_new(TfsInodeType type) {
	switch (type) {
		// Set initial data to `NULL`
		case TfsInodeTypeFile:
			return (TfsInode){
				.type = type,
				.data = {.file = {.contents = NULL}},
			};

		case TfsInodeTypeDir:
			return (TfsInode){
				.type = type,
				.data = {.dir = tfs_inode_dir_new()},
			};

		case TfsInodeTypeNone:
		default:
			return (TfsInode){
				.type = type,
			};
	}
}

void tfs_inode_destroy(TfsInode* self) {
	switch (self->type) {
		// Deallocate a file's contents
		case TfsInodeTypeFile:
			free(self->data.file.contents);
			break;

		// Deallocate a directory's children
		case TfsInodeTypeDir:
			tfs_inode_dir_destroy(&self->data.dir);
			break;

		case TfsInodeTypeNone:
		default:
			break;
	}
}
