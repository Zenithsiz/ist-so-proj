#include "table.h"

// Includes
#include <stdio.h>	// stderr, fprintf
#include <stdlib.h> // exit, EXIT_FAILURE
#include <string.h> // strlen, strncpy

TfsInodeTable tfs_inode_table_new(size_t max_inodes) {
	// Allocate the inode table
	TfsInode* inodes = malloc(sizeof(TfsInode) * max_inodes);

	// If an error occurred, exit
	if (inodes == NULL) {
		fprintf(stderr, "Unable to allocate inode table");
		exit(EXIT_FAILURE);
	}

	// Else assign each table to an empty inode
	TfsInodeTable table = {.inodes = inodes, .len = max_inodes};
	for (size_t n = 0; n < table.len; n++) {
		tfs_inode_init(&table.inodes[n], TfsInodeTypeNone);
	}

	return table;
}

void tfs_inode_table_drop(TfsInodeTable* table) {
	// Drop each inode
	for (size_t n = 0; n < table->len; n++) {
		tfs_inode_drop(&table->inodes[n]);
	}

	// And free the inode table
	free(table->inodes);
}

TfsInodeTableCreateError tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type, TfsInodeIdx* idx, TfsInodeData** data) {
	for (TfsInodeIdx n = 0; n < table->len; n++) {
		// Skip all non empty inodes
		if (table->inodes[n].type != TfsInodeTypeNone) {
			continue;
		}

		// Initialize the node
		tfs_inode_init(&table->inodes[n], type);

		// Set the index and data and return
		if (idx != NULL) {
			*idx = n;
		}
		if (data != NULL) {
			*data = &table->inodes[n].data;
		}
		return TfsInodeTableCreateErrorSuccess;
	}

	// If we got here, the inode table was full
	return TfsInodeTableCreateErrorFull;
}

TfsInodeTableRemoveError tfs_inode_table_remove(TfsInodeTable* table, TfsInodeIdx idx) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableRemoveErrorInvalidIdx;
	}

	// Drop the node and replace it with an empty node
	tfs_inode_drop(&table->inodes[idx]);
	tfs_inode_init(&table->inodes[idx], TfsInodeTypeNone);

	return TfsInodeTableRemoveErrorSuccess;
}

TfsInodeTableGetError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
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
				if (table->inodes[idx].data.dir.entries[n].inode_idx == (TfsInodeIdx)TfsInodeIdxNone) {
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
