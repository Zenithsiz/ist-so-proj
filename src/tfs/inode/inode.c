#include "inode.h"

TfsInode tfs_inode_new(TfsInodeType type, TfsLockKind lock_kind) {
	switch (type) {
		// Set initial data to `NULL`
		case TfsInodeTypeFile:
			return (TfsInode){
				.type = type,
				.data = {.file = {.contents = NULL}},
				.lock = tfs_lock_new(lock_kind),
			};

		case TfsInodeTypeDir:
			return (TfsInode){
				.type = type,
				.data = {.dir = tfs_inode_dir_new()},
				.lock = tfs_lock_new(lock_kind),
			};

		case TfsInodeTypeNone:
		default:
			return (TfsInode){
				.type = type,
				.lock = tfs_lock_new(lock_kind),
			};
	}
}

void tfs_inode_destroy(TfsInode* self) {
	// Deallocate the lock
	tfs_lock_destroy(&self->lock);

	// And empty this node
	tfs_inode_empty(self);
}

void tfs_inode_empty(TfsInode* self) {
	switch (self->type) {
		// Deallocate a file's contents
		// Note: Fine to pass `NULL` here.
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

	self->type = TfsInodeTypeNone;
}
