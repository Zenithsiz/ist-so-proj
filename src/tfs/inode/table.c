#include "table.h"

// Includes
#include <stdio.h>	// stderr, fprintf
#include <stdlib.h> // exit, EXIT_FAILURE
#include <string.h> // strlen, strncpy

TfsInodeTable tfs_inode_table_new(void) {
	return (TfsInodeTable){
		// Note: `NULL` can be safely passed to both `free` and `realloc`.
		.inodes	  = NULL,
		.capacity = 0,
	};
}

void tfs_inode_table_drop(TfsInodeTable* table) {
	// Drop each inode
	for (size_t n = 0; n < table->capacity; n++) {
		tfs_inode_drop(&table->inodes[n]);
	}

	// And free the inode table
	// Note: It's fine if `inodes` is `NULL`.
	free(table->inodes);
}

TfsInodeTableCreateReturn tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type) {
	// Find the first non-empty node
	TfsInodeIdx empty_idx = TFS_INODE_IDX_NONE;
	for (TfsInodeIdx n = 0; n < table->capacity; n++) {
		if (table->inodes[n].type == TfsInodeTypeNone) {
			empty_idx = n;
			break;
		}
	}

	// If we didn't find it, reallocate
	if (empty_idx == TFS_INODE_IDX_NONE) {
		// Double the current capacity so we don't allocate often
		// Note: We start by allocating 4 to skip having to
		//       the smaller 1 and 2 allocations.
		size_t new_capacity = table->capacity == 0 ? 4 : 2 * table->capacity;

		// Try to allocate
		// Note: It's fine even if `table->inodes` is `NULL`
		TfsInode* new_inodes = realloc(table->inodes, new_capacity * sizeof(TfsInode));
		if (new_inodes == NULL) {
			fprintf(stderr, "Unable to expand inode table capacity to %u", new_capacity);
			exit(EXIT_FAILURE);
		}

		// Set all new inodes as empty
		// Note: We skip the first, as we'll initialize it after this.
		for (size_t n = table->capacity + 1; n < new_capacity; n++) {
			new_inodes[n] = tfs_inode_new(TfsInodeTypeNone);
		}

		// Set the index as the first new index and continue
		empty_idx = table->capacity;

		// Move everything into `dir`
		table->inodes	= new_inodes;
		table->capacity = new_capacity;
	}

	// Then initialize the node
	table->inodes[empty_idx] = tfs_inode_new(type);

	// And return it
	return (TfsInodeTableCreateReturn){
		.idx  = empty_idx,
		.data = &table->inodes[empty_idx].data,
	};
}

TfsInodeTableRemoveError tfs_inode_table_remove(TfsInodeTable* table, TfsInodeIdx idx) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->capacity || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableRemoveErrorInvalidIdx;
	}

	// Drop the node and replace it with an empty node
	tfs_inode_drop(&table->inodes[idx]);
	table->inodes[idx] = tfs_inode_new(TfsInodeTypeNone);

	return TfsInodeTableRemoveErrorSuccess;
}

TfsInodeTableGetError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->capacity || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableGetErrorInvalidIdx;
	}

	if (type != NULL)
		*type = table->inodes[idx].type;

	if (data != NULL)
		*data = &table->inodes[idx].data;

	return TfsInodeTableGetErrorSuccess;
}

void tfs_inode_table_print_tree(TfsInodeTable* table, FILE* out, TfsInodeIdx idx, const char* cur_path) {
	switch (table->inodes[idx].type) {
		// For files, just print it's path and return
		case TfsInodeTypeFile: {
			fprintf(out, "%s\n", cur_path);
			break;
		}

		// For directories, print their path and then print the tree of their children
		case TfsInodeTypeDir: {
			fprintf(out, "%s\n", cur_path);
			for (size_t n = 0; n < table->inodes[idx].data.dir.capacity; n++) {
				// If this entry is empty, skip
				if (table->inodes[idx].data.dir.entries[n].inode_idx == TFS_INODE_IDX_NONE) {
					continue;
				}

				// Else build the path
				char child_path[TFS_DIR_MAX_FILE_NAME_LEN];
				if (snprintf(child_path, sizeof(child_path), "%s/%s", cur_path, table->inodes[idx].data.dir.entries[n].name) > (int)sizeof(child_path)) {
					fprintf(stderr, "truncation when building full path\n");
				}

				// And recurse for this entry.
				tfs_inode_table_print_tree(table, out, table->inodes[idx].data.dir.entries[n].inode_idx, child_path);
			}
			break;
		}

		case TfsInodeTypeNone:
		default: {
			break;
		}
	}
}
