#include "table.h"

// Includes
#include <stdio.h>	  // stderr, fprintf
#include <stdlib.h>	  // exit, EXIT_FAILURE
#include <string.h>	  // strlen, strncpy
#include <tfs/log.h>  // DEBUG_LOG
#include <tfs/util.h> // TFS_MIN, TFS_MAX

TfsInodeTable tfs_inode_table_new(void) {
	return (TfsInodeTable){
		// Note: `NULL` can be safely passed to both `free` and `realloc`.
		.inodes	  = NULL,
		.capacity = 0,
	};
}

void tfs_inode_table_drop(TfsInodeTable* self) {
	// Drop each inode
	for (size_t n = 0; n < self->capacity; n++) {
		tfs_inode_drop(&self->inodes[n]);
	}

	// And free the inode table
	// Note: It's fine if `inodes` is `NULL`.
	free(self->inodes);
}

TfsInodeTableCreateReturn tfs_inode_table_create(TfsInodeTable* self, TfsInodeType type) {
	TFS_DEBUG_LOG("'%p': Creating new inode (Type %s)", (void*)self, tfs_inode_type_str(type));

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
		// Double the current capacity so we don't allocate often
		// Note: We allocate at least 4 because `2 + 0 == 0`.
		size_t new_capacity = TFS_MAX((size_t)4, 2 * self->capacity);

		// Try to allocate
		// Note: It's fine even if `table->inodes` is `NULL`
		TFS_DEBUG_LOG("'%p': Expanding entries from %zu to %zu", (void*)self, self->capacity, new_capacity);
		TfsInode* new_inodes = realloc(self->inodes, new_capacity * sizeof(TfsInode));
		if (new_inodes == NULL) {
			fprintf(stderr, "Unable to expand inode table capacity to %zu\n", new_capacity);
			exit(EXIT_FAILURE);
		}

		// Set all new inodes as empty
		// Note: We skip the first, as we'll initialize it after this.
		TFS_DEBUG_LOG("'%p': Setting entries %zu..%zu as empty", (void*)self, self->capacity + 1, new_capacity);
		for (size_t n = self->capacity + 1; n < new_capacity; n++) {
			new_inodes[n] = tfs_inode_new(TfsInodeTypeNone);
		}

		// Set the index as the first new index and continue
		empty_idx = self->capacity;

		// Move everything into `dir`
		self->inodes   = new_inodes;
		self->capacity = new_capacity;
	}

	// Then initialize the node
	TFS_DEBUG_LOG("'%p': Initializing new entry %zu", (void*)self, empty_idx);
	self->inodes[empty_idx] = tfs_inode_new(type);

	// And return it
	return (TfsInodeTableCreateReturn){
		.idx  = empty_idx,
		.data = &self->inodes[empty_idx].data,
	};
}

TfsInodeTableRemoveError tfs_inode_table_remove(TfsInodeTable* self, TfsInodeIdx idx) {
	TFS_DEBUG_LOG("'%p': Removing inode %zu", (void*)self, idx);

	// If it's out of bounds, or empty, return Err
	if (idx >= self->capacity || self->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableRemoveErrorInvalidIdx;
	}

	// Drop the node and replace it with an empty node
	tfs_inode_drop(&self->inodes[idx]);
	self->inodes[idx] = tfs_inode_new(TfsInodeTypeNone);

	return TfsInodeTableRemoveErrorSuccess;
}

TfsInodeTableGetError tfs_inode_table_get(TfsInodeTable* self, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data) {
	// If it's out of bounds, or empty, return Err
	if (idx >= self->capacity || self->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableGetErrorInvalidIdx;
	}

	if (type != NULL)
		*type = self->inodes[idx].type;

	if (data != NULL)
		*data = &self->inodes[idx].data;

	return TfsInodeTableGetErrorSuccess;
}

void tfs_inode_table_print_tree(TfsInodeTable* self, FILE* out, TfsInodeIdx idx, const char* cur_path) {
	// Print it's path
	fprintf(out, "Path: '%s%s'\n", cur_path, self->inodes[idx].type == TfsInodeTypeDir ? "/" : "");

	// Print it's inode
	fprintf(out, "\tInode: %zu\n", idx);

	// Print it's type
	fprintf(out, "\tType: %s\n", tfs_inode_type_str(self->inodes[idx].type));

	// Print it's data
	switch (self->inodes[idx].type) {
		case TfsInodeTypeFile: {
			// TODO: Print file length
			break;
		}

		case TfsInodeTypeDir: {
			// Print the capacity for this directory
			fprintf(out, "\tCapacity: %zu\n", self->inodes[idx].data.dir.capacity);

			for (size_t n = 0; n < self->inodes[idx].data.dir.capacity; n++) {
				// If this entry is empty, skip
				if (self->inodes[idx].data.dir.entries[n].inode_idx == TFS_INODE_IDX_NONE) {
					continue;
				}

				// Else build the path
				char child_path[TFS_DIR_MAX_FILE_NAME_LEN];
				if (snprintf(child_path, sizeof(child_path), "%s/%s", cur_path, self->inodes[idx].data.dir.entries[n].name) > (int)sizeof(child_path)) {
					fprintf(stderr, "truncation when building full path\n");
				}

				// And recurse for this entry.
				tfs_inode_table_print_tree(self, out, self->inodes[idx].data.dir.entries[n].inode_idx, child_path);
			}
			break;
		}

		case TfsInodeTypeNone:
		default: {
			break;
		}
	}
}
