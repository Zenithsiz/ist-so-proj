#include <inode/table.h>

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

TfsInodeTableError tfs_inode_table_create(TfsInodeTable* table, TfsInodeType type, TfsInodeIdx* idx) {
	for (TfsInodeIdx n = 0; n < table->len; n++) {
		// Skip all non empty inodes
		if (table->inodes[n].type != TfsInodeTypeNone) {
			continue;
		}

		// Initialize the node
		tfs_inode_init(&table->inodes[n], type);

		// Set the index and return
		if (idx != NULL) {
			*idx = n;
		}
		return TfsInodeTableErrorSuccess;
	}

	// If we got here, the inode table was full
	return TfsInodeTableErrorInodeTableFull;
}

TfsInodeTableError tfs_inode_table_delete(TfsInodeTable* table, TfsInodeIdx idx) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeIdxOutOfBounds;
	}

	// Drop the node and replace it with an empty node
	tfs_inode_drop(&table->inodes[idx]);
	tfs_inode_init(&table->inodes[idx], TfsInodeTypeNone);

	return TfsInodeTableErrorSuccess;
}

TfsInodeTableError tfs_inode_table_get(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData* data) {
	// If it's out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeIdxOutOfBounds;
	}

	if (type != NULL)
		*type = table->inodes[idx].type;

	if (data != NULL)
		*data = table->inodes[idx].data;

	return TfsInodeTableErrorSuccess;
}

/*
TfsInodeTableError tfs_inode_table_set_file(TfsInodeTable* table, TfsInodeIdx idx, char *fileContents, size_t len)
{
	()
}
*/
TfsInodeTableError tfs_inode_table_dir_reset_entry(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeIdx sub_idx) {
	// If either indexes are out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeIdxOutOfBounds;
	}

	if (sub_idx >= table->len || table->inodes[sub_idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeSubIdxOutOfBounds;
	}

	// If it's not a directory, return Err
	if (table->inodes[idx].type != TfsInodeTypeDir) {
		return TfsInodeTableErrorInodeIdxNotDir;
	}

	// Find the entry with index `sub_idx`
	for (int n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (table->inodes[idx].data.dir.entries[n].inode_idx == sub_idx) {
			// Then set it to none and remove it's name
			table->inodes[idx].data.dir.entries[n].inode_idx = TfsInodeIdxNone;
			table->inodes[idx].data.dir.entries[n].name[0]	 = '\0';
			return TfsInodeTableErrorSuccess;
		}
	}

	// If we got here, there was no entry with the sub index, so return Err
	return TfsInodeTableErrorSubIdxNotFoundInDirEntries;
}
TfsInodeTableError tfs_inode_table_dir_add_entry(TfsInodeTable* table, TfsInodeIdx idx, TfsInodeIdx sub_idx, const char* sub_name, size_t sub_name_len) {
	// If either indexes are out of bounds, or empty, return Err
	if (idx >= table->len || table->inodes[idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeIdxOutOfBounds;
	}

	if (sub_idx >= table->len || table->inodes[sub_idx].type == TfsInodeTypeNone) {
		return TfsInodeTableErrorInodeSubIdxOutOfBounds;
	}

	// If it's not a directory, return Err
	if (table->inodes[idx].type != TfsInodeTypeDir) {
		return TfsInodeTableErrorInodeIdxNotDir;
	}

	// If the name is empty, return Err
	if (sub_name_len == 0) {
		return TfsInodeTableErrorDirEntryNameEmpty;
	}

	// Else search for an empty directory entry
	for (int n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (table->inodes[idx].data.dir.entries[n].inode_idx == TfsInodeIdxNone) {
			// Set it's index and copy the name
			table->inodes[idx].data.dir.entries[n].inode_idx = sub_idx;
			strncpy(table->inodes[idx].data.dir.entries[n].name, sub_name, sub_name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : sub_name_len);
			return TfsInodeTableErrorSuccess;
		}
	}

	// If we got here, there was no empty entry
	return TfsInodeTableErrorDirFull;
}
TfsInodeTableError tfs_inode_table_print_tree(TfsInodeTable* table, FILE* fp, TfsInodeIdx idx, const char* name) {
	switch (table->inodes[idx].type) {
		// For files, just print it's name and return
		case TfsInodeTypeFile: {
			fprintf(fp, "%s\n", name);
			break;
		}

		// For directories, print their name and then print the tree of their children
		case TfsInodeTypeDir: {
			fprintf(fp, "%s\n", name);
			for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
				if (table->inodes[idx].data.dir.entries[n].inode_idx != TfsInodeIdxNone) {
					char path[TFS_DIR_MAX_FILE_NAME_LEN];
					if (snprintf(path, sizeof(path), "%s/%s", name, table->inodes[idx].data.dir.entries[n].name) > (int)sizeof(path)) {
						fprintf(stderr, "truncation when building full path\n");
					}
					tfs_inode_table_print_tree(table, fp, table->inodes[idx].data.dir.entries[n].inode_idx, path);
				}
			}
			break;
		}

		case TfsInodeTypeNone:
		default: {
			break;
		}
	}

	return TfsInodeTableErrorSuccess;
}
