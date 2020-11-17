#include "inode.h"

// Imports
#include <stdlib.h> // free

TfsInode tfs_inode_new(void) {
	return (TfsInode){
		.type = TfsInodeTypeNone,
		.lock = tfs_rw_lock_new(),
	};
}
void tfs_inode_init(TfsInode* self, TfsInodeType type) {
	tfs_inode_empty(self);

	switch (type) {
		// Set initial data to `NULL`
		case TfsInodeTypeFile: {
			self->data.file.contents = NULL;
			break;
		}
		case TfsInodeTypeDir: {
			self->data.dir = tfs_inode_dir_new();
			break;
		}
		case TfsInodeTypeNone: {
			break;
		}
		default: {
			break;
		}
	}
	self->type = type;
}

void tfs_inode_destroy(TfsInode* self) {
	// Lock our lock for unique access
	tfs_rw_lock_lock(&self->lock, TfsRwLockAccessUnique);

	// Empty this node
	tfs_inode_empty(self);

	// Then unlock and destroy the lock.
	tfs_rw_lock_unlock(&self->lock);
	tfs_rw_lock_destroy(&self->lock);
}

void tfs_inode_empty(TfsInode* self) {
	switch (self->type) {
		case TfsInodeTypeFile: {
			// Note: Fine to pass `NULL` here.
			free(self->data.file.contents);
			break;
		}

		case TfsInodeTypeDir: {
			tfs_inode_dir_destroy(&self->data.dir);
			break;
		}

		default:
		case TfsInodeTypeNone: {
			break;
		}
	}

	self->type = TfsInodeTypeNone;
}
