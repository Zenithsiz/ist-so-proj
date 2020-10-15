#include "table.h"

// Includes
#include <stdio.h>	  // stderr, fprintf
#include <stdlib.h>	  // exit, EXIT_FAILURE
#include <string.h>	  // strlen, strncpy
#include <tfs/util.h> // tfs_min_size_t

/// @brief Reallocates the inode table.
/// @return Index of an uninitialized inode.
/// @details
/// All new elements are set to be empty, with
/// the exception of the returned element.
/// If unable to reallocate the table, this function
/// will call `exit` with `EXIT_FAILURE`.
static TfsInodeIdx tfs_inode_table_realloc(TfsInodeTable* self) {
	// Double the current capacity so we don't allocate often
	// Note: We allocate at least 4 because `2 * 0 == 0`.
	size_t new_capacity = tfs_max_size_t(4, 2 * self->capacity);

	// Try to allocate
	// Note: We can pass `NULL` to `realloc`.
	TfsInode* new_inodes = realloc(self->inodes, new_capacity * sizeof(TfsInode));
	if (new_inodes == NULL) {
		fprintf(stderr, "Unable to expand inode table capacity to %zu\n", new_capacity);
		exit(EXIT_FAILURE);
	}

	// Set all new inodes as empty, except the first.
	for (size_t n = self->capacity + 1; n < new_capacity; n++) {
		new_inodes[n] = tfs_inode_new(TfsInodeTypeNone, self->lock_kind);
	}

	// Get the empty index as the first new inode.
	TfsInodeIdx empty_idx = self->capacity;

	// Move everything into `self`
	self->inodes   = new_inodes;
	self->capacity = new_capacity;

	// And return the empty index
	return empty_idx;
}

TfsInodeTable tfs_inode_table_new(TfsLockKind lock_kind) {
	return (TfsInodeTable){
		.inodes	   = NULL,
		.capacity  = 0,
		.lock_kind = lock_kind,
		.rw_lock   = tfs_lock_new(TfsLockKindRWLock),
	};
}

void tfs_inode_table_destroy(TfsInodeTable* self) {
	// Lock our rwlock for unique access, so we wait for
	// any borrows to end
	tfs_lock_lock(&self->rw_lock, TfsLockAccessUnique);

	// Destroy each inode
	for (size_t n = 0; n < self->capacity; n++) {
		tfs_inode_destroy(&self->inodes[n]);
	}

	// Then unlock and destroy the lock
	tfs_lock_unlock(&self->rw_lock);
	tfs_lock_destroy(&self->rw_lock);

	// Free the inode table and set it to NULL
	// Note: We can pass `NULL` here safely.
	free(self->inodes);
	self->inodes = NULL;
}

TfsInodeIdx tfs_inode_table_add(TfsInodeTable* self, TfsInodeType type, TfsLockAccess access) {
	// Lock our rwlock for unique access
	tfs_lock_lock(&self->rw_lock, TfsLockAccessUnique);

	// Find the first non-empty node
	TfsInodeIdx empty_idx = TFS_INODE_IDX_NONE;
	for (TfsInodeIdx n = 0; n < self->capacity; n++) {
		if (self->inodes[n].type == TfsInodeTypeNone) {
			empty_idx = n;
			break;
		}
	}

	// If we didn't find it, reallocate
	if (empty_idx == TFS_INODE_IDX_NONE) {
		empty_idx = tfs_inode_table_realloc(self);
	}

	// Then initialize and lock the inode
	self->inodes[empty_idx] = tfs_inode_new(type, self->lock_kind);
	tfs_lock_lock(&self->inodes[empty_idx].lock, access);

	// Then downgrade our write lock to read
	// Note: Although this isn't atomic, it won't affect the newly
	//       created inode, as we just return an index, so the table
	//       may reallocate without worry.
	tfs_lock_unlock(&self->rw_lock);
	tfs_lock_lock(&self->rw_lock, TfsLockAccessShared);

	// Unlock and return.
	return empty_idx;
}

bool tfs_inode_table_lock(TfsInodeTable* self, TfsInodeIdx idx, TfsLockAccess access, TfsInodeType* type, TfsInodeData** data) {
	// If the index is out of bounds, or empty, return error
	if (idx >= self->capacity || self->inodes[idx].type == TfsInodeTypeNone) {
		return false;
	}

	// The lock ourselves and the inode
	// Note: We keep our lock locked for shared access until
	//       the user returns it to `unlock_inode`.
	tfs_lock_lock(&self->rw_lock, TfsLockAccessShared);
	tfs_lock_lock(&self->inodes[idx].lock, access);

	// Set it's data and return
	// SAFETY: We and the inode are locked
	if (type != NULL) { *type = self->inodes[idx].type; }
	if (data != NULL) { *data = &self->inodes[idx].data; }
	return true;
}

bool tfs_inode_table_unlock_inode(TfsInodeTable* self, TfsInodeIdx idx) {
	// If the index is out of bounds, or empty, return error
	if (idx >= self->capacity || self->inodes[idx].type == TfsInodeTypeNone) {
		return false;
	}

	// Else unlock the inode and unlock our rwlock
	tfs_lock_unlock(&self->inodes[idx].lock);
	tfs_lock_unlock(&self->rw_lock);
	return true;
}

bool tfs_inode_table_remove_inode(TfsInodeTable* self, TfsInodeIdx idx) {
	// If the index is out of bounds, or empty, return error
	if (idx >= self->capacity || self->inodes[idx].type == TfsInodeTypeNone) {
		return false;
	}

	// Else empty the inode, unlock it and unlock our rwlock
	tfs_inode_empty(&self->inodes[idx]);
	tfs_lock_unlock(&self->inodes[idx].lock);
	tfs_lock_unlock(&self->rw_lock);
	return true;
}

void tfs_inode_table_print_tree(const TfsInodeTable* self, TfsInodeIdx idx, FILE* out, const char* path) {
	// Print it's path
	fprintf(out, "%s\n", path);

	// If it's a directory, print it's child
	if (self->inodes[idx].type == TfsInodeTypeDir) {
		TfsInodeDir* dir = &self->inodes[idx].data.dir;
		for (size_t n = 0; n < dir->capacity; n++) {
			// If this entry is empty, skip
			if (dir->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
				continue;
			}

			// Else build the path
			// Note: This checks how big we need to make the buffer to fit the
			//       current path and the child's name
			int path_size = snprintf(NULL, 0, "%s/%.*s", path, (int)dir->entries[n].name_len, dir->entries[n].name);
			if (path_size < 0) {
				fprintf(stderr, "Unable to get child path buffer size\n");
				exit(EXIT_FAILURE);
			}
			char child_path[path_size + 1];
			if (snprintf(child_path, sizeof(child_path), "%s/%.*s", path, (int)dir->entries[n].name_len, dir->entries[n].name) < 0) {
				fprintf(stderr, "Unable to format child path buffer\n");
				exit(EXIT_FAILURE);
			}

			// And recurse for this entry.
			tfs_inode_table_print_tree(self, dir->entries[n].inode_idx, out, child_path);
		}
	}
}
