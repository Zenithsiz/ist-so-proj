#include "fs.h"

// Includes
#include <assert.h> // assert
#include <stdio.h>	// stderr, fprintf
#include <string.h> // strcpy

TfsFs tfs_fs_new(size_t max_inodes) {
	// Create the inode table
	TfsFs fs = {.inode_table = tfs_inode_table_new(max_inodes)};

	// Create the root node at index `0`.
	TfsInodeIdx root;
	if (tfs_inode_table_create(&fs.inode_table, TfsInodeTypeDir, &root, NULL) != TfsInodeTableCreateErrorSuccess || root != 0) {
		fprintf(stderr, "Failed to create root");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void tfs_fs_drop(TfsFs* fs) {
	// Drop the inode table
	tfs_inode_table_drop(&fs->inode_table);
}

TfsFsCreateError tfs_fs_create(TfsFs* fs, TfsInodeType type, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath name;
	tfs_path_split_last(path, &parent_path, &name);

	// If we can't find the parent directory, return Err
	TfsInodeIdx parent_idx;
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	if (tfs_fs_find(fs, parent_path, &parent_idx, &parent_type, &parent_data) != TfsFsFindErrorSuccess) {
		return (TfsFsCreateError){.kind = TfsFsCreateErrorInexistentParentDirectory};
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return (TfsFsCreateError){.kind = TfsFsCreateErrorParentNotDir};
	}

	// If there's an entry with the same name, return Err
	if (tfs_inode_dir_search_by_name(&parent_data->dir, name.chars, name.len, NULL) == TfsInodeDirErrorSuccess) {
		return (TfsFsCreateError){.kind = TfsFsCreateErrorDuplicateName};
	}

	// Else create the inode
	TfsInodeIdx idx;
	TfsInodeTableCreateError create_err = tfs_inode_table_create(&fs->inode_table, type, &idx, NULL);
	if (create_err != TfsInodeTableCreateErrorSuccess) {
		return (TfsFsCreateError){.kind = TfsFsCreateErrorCreateInode, .data = {.create_inode = create_err}};
	}

	// And add it to the directory
	TfsInodeDirError add_entry_err = tfs_inode_dir_add_entry(&parent_data->dir, idx, name.chars, name.len);
	if (add_entry_err != TfsInodeDirErrorSuccess) {
		// Note: If unable to, we delete the inode we just created.
		assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);
		return (TfsFsCreateError){.kind = TfsFsCreateErrorAddEntry, .data = {.add_entry = add_entry_err}};
	}

	return (TfsFsCreateError){.kind = TfsFsCreateErrorSuccess};
}

TfsFsRemoveError tfs_fs_remove(TfsFs* fs, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath name;
	tfs_path_split_last(path, &parent_path, &name);

	// If we can't find the parent directory, return Err
	TfsInodeIdx parent_idx;
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	if (tfs_fs_find(fs, parent_path, &parent_idx, &parent_type, &parent_data) != TfsFsFindErrorSuccess) {
		return TfsFsRemoveErrorInexistentParentDirectory;
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return TfsFsRemoveErrorParentNotDir;
	}

	// If there isn't an entry with the name, return Err
	TfsInodeIdx idx;
	if (tfs_inode_dir_search_by_name(&parent_data->dir, name.chars, name.len, &idx) != TfsInodeDirErrorSuccess) {
		return TfsFsRemoveErrorNameNotFound;
	}

	// Get the type and data of the node to delete
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_get(&fs->inode_table, idx, &type, &data) == TfsInodeTableGetErrorSuccess);

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		return TfsFsRemoveErrorRemoveNonEmptyDir;
	}

	// Else remove it from the directory
	assert(tfs_inode_dir_remove_entry(&parent_data->dir, idx) == TfsInodeDirErrorSuccess);

	// And delete it from the inode table
	assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);

	return TfsFsRemoveErrorSuccess;
}

TfsFsFindError tfs_fs_find(TfsFs* fs, TfsPath path, TfsInodeIdx* idx, TfsInodeType* type, TfsInodeData** data) {
	// Current inode index and data
	TfsInodeIdx cur_idx = 0;
	TfsInodeData* cur_data;
	TfsInodeType cur_type;

	do {
		// If there's no more path to split, return the current inode
		if (path.len == 0) {
			if (idx != NULL) {
				*idx = cur_idx;
			}
			if (type != NULL) {
				*type = cur_type;
			}
			if (data != NULL) {
				*data = cur_data;
			}
			return TfsFsFindErrorSuccess;
		}

		// Get the current inode's type and data
		assert(tfs_inode_table_get(&fs->inode_table, cur_idx, &cur_type, &cur_data) == TfsInodeTableGetErrorSuccess);

		// Else, if this isn't a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			return TfsFsFindErrorParentsNotDir;
		}

		// Get the name of the current inode we're in and skip it.
		TfsPath cur_path;
		tfs_path_split_first(path, &cur_path, &path);

		// Try to get the node
		if (tfs_inode_dir_search_by_name(&cur_data->dir, cur_path.chars, cur_path.len, &cur_idx) != TfsInodeDirErrorSuccess) {
			return TfsFsFindErrorNameNotFound;
		}
	} while (1);
}

void tfs_fs_print(TfsFs* fs, FILE* out) {
	tfs_inode_table_print_tree(&fs->inode_table, out, 0, "");
}
