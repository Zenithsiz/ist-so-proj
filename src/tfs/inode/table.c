#include "table.h"

// Includes
#include <stdio.h>	  // stderr, fprintf
#include <stdlib.h>	  // exit, EXIT_FAILURE
#include <string.h>	  // strlen, strncpy
#include <tfs/util.h> // tfs_min_size_t

TfsInodeTable tfs_inode_table_new(size_t size) {
	// Create all inodes
	TfsInode* inodes = malloc(size * sizeof(TfsInode));
	if (inodes == NULL) {
		fprintf(stderr, "Unable to allocate inode table for %zu inodes", size);
		exit(EXIT_FAILURE);
	}

	// Initialize them to empty
	for (TfsInodeIdx n = 0; n < size; n++) { inodes[n] = tfs_inode_new(TfsInodeTypeNone); }

	return (TfsInodeTable){
		.inodes = inodes,
		.capacity = size,
	};
}

void tfs_inode_table_destroy(TfsInodeTable* const self) {
	// Destroy each inode
	for (TfsInodeIdx n = 0; n < self->capacity; n++) { tfs_inode_destroy(&self->inodes[n]); }

	// Free the inode table and set it to NULL.
	free(self->inodes);
	self->inodes = NULL;
}

TfsInodeIdx tfs_inode_table_add(TfsInodeTable* const self, TfsInodeType type) {
	// Make sure we're not creating an empty inode
	assert(type != TfsInodeTypeNone);

	// Find the first non-empty node
	// Note: This keeps the inode locked for when we find it
	TfsInodeIdx empty_idx = TFS_INODE_IDX_NONE;
	for (TfsInodeIdx n = 0; n < self->capacity; n++) {
		// If we couldn't lock it, continue
		if (!tfs_rw_lock_try_lock(&self->inodes[n].lock, TfsRwLockAccessUnique)) { continue; }

		// If it's empty, set it as our empty index and break
		if (self->inodes[n].type == TfsInodeTypeNone) {
			empty_idx = n;
			break;
		}

		// Else unlock and continue
		tfs_rw_lock_unlock(&self->inodes[n].lock);
	}

	// Make sure we found some
	assert(empty_idx != TFS_INODE_IDX_NONE);

	// Then initialize it
	self->inodes[empty_idx] = tfs_inode_new(type);

	// And return it
	return empty_idx;
}

TfsLockedInode tfs_inode_table_lock(TfsInodeTable* const self, TfsInodeIdx idx, TfsRwLockAccess access) {
	// Make sure the index is valid.
	assert(idx < self->capacity);

	// Lock the inode
	tfs_rw_lock_lock(&self->inodes[idx].lock, access);

	// Make sure it's not empty
	assert(self->inodes[idx].type != TfsInodeTypeNone);

	// Then return the inode, locked
	return (TfsLockedInode){
		.idx = idx,
		.type = self->inodes[idx].type,
		.data = &self->inodes[idx].data,
	};
}

void tfs_inode_table_unlock_inode(TfsInodeTable* const self, TfsInodeIdx idx) {
	// Make sure the index is valid and non-empty
	assert(idx < self->capacity);
	assert(self->inodes[idx].type != TfsInodeTypeNone);

	// Unlock the inode
	tfs_rw_lock_unlock(&self->inodes[idx].lock);
}

void tfs_inode_table_remove_inode(TfsInodeTable* const self, TfsInodeIdx idx) {
	// Make sure the index is valid and non-empty
	assert(idx < self->capacity);

	// Set the inode to be empty and unlock it.
	tfs_inode_empty(&self->inodes[idx]);
	tfs_rw_lock_unlock(&self->inodes[idx].lock);
}

void tfs_inode_table_print_tree(
	const TfsInodeTable* const self, TfsInodeIdx idx, FILE* const out, const char* const path) {
	// Make sure the index is valid and non-empty
	assert(idx < self->capacity);
	assert(self->inodes[idx].type != TfsInodeTypeNone);

	// Print it's path
	fprintf(out, "%s\n", path);

	// If it's a directory, print it's child
	if (self->inodes[idx].type == TfsInodeTypeDir) {
		TfsInodeDir* dir = &self->inodes[idx].data.dir;
		for (size_t n = 0; n < dir->capacity; n++) {
			// If this entry is empty, skip
			if (dir->entries[n].inode_idx == TFS_INODE_IDX_NONE) { continue; }

			// Else build the path
			// Note: This checks how big we need to make the buffer to fit the
			//       current path and the child's name
			int path_size = snprintf(NULL, 0, "%s/%.*s", path, (int)dir->entries[n].name_len, dir->entries[n].name);
			if (path_size < 0) {
				fprintf(stderr, "Unable to get child path buffer size\n");
				exit(EXIT_FAILURE);
			}
			char child_path[path_size + 1];
			if (snprintf(child_path,
					sizeof(child_path),
					"%s/%.*s",
					path,
					(int)dir->entries[n].name_len,
					dir->entries[n].name) < 0) {
				fprintf(stderr, "Unable to format child path buffer\n");
				exit(EXIT_FAILURE);
			}

			// And recurse for this entry.
			tfs_inode_table_print_tree(self, dir->entries[n].inode_idx, out, child_path);
		}
	}
}
