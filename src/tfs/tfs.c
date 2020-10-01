#include "tfs.h"

// Includes
#include <assert.h> // assert
#include <stdio.h>	// stderr, fprintf
#include <string.h> // strcpy

TfsFileSystem tfs_new(size_t max_inodes) {
	// Create the inode table
	TfsFileSystem fs = {.inode_table = tfs_inode_table_new(max_inodes)};

	// Create the root node at index `0`.
	TfsInodeIdx root;
	if (tfs_inode_table_create(&fs.inode_table, TfsInodeTypeDir, &root, NULL) != TfsInodeTableCreateErrorSuccess || root != 0) {
		fprintf(stderr, "Failed to create root");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void tfs_drop(TfsFileSystem* fs) {
	// Drop the inode table
	tfs_inode_table_drop(&fs->inode_table);
}

TfsFileSystemCreateError tfs_create(TfsFileSystem* fs, TfsInodeType type, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath name;
	tfs_path_split_last(path, &parent_path, &name);

	// If we can't find the parent directory, return Err
	TfsInodeIdx parent_idx;
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	if (tfs_find(fs, parent_path, &parent_idx, &parent_type, &parent_data) != TfsFileSystemFindErrorSuccess) {
		return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorInexistentParentDirectory};
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorParentNotDir};
	}

	// If there's an entry with the same name, return Err
	if (tfs_inode_dir_search_by_name(&parent_data->dir, name.chars, name.len, NULL) == TfsInodeDirErrorSuccess) {
		return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorDuplicateName};
	}

	// Else create the inode
	TfsInodeIdx idx;
	TfsInodeTableCreateError create_err = tfs_inode_table_create(&fs->inode_table, type, &idx, NULL);
	if (create_err != TfsInodeTableCreateErrorSuccess) {
		return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorCreateInode, .data = {.create_inode = create_err}};
	}

	// And add it to the directory
	TfsInodeDirError add_entry_err = tfs_inode_dir_add_entry(&parent_data->dir, idx, name.chars, name.len);
	if (add_entry_err != TfsInodeDirErrorSuccess) {
		// Note: If unable to, we delete the inode we just created.
		assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);
		return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorAddEntry, .data = {.add_entry = add_entry_err}};
	}

	return (TfsFileSystemCreateError){.kind = TfsFileSystemCreateErrorSuccess};
}

TfsFileSystemRemoveError tfs_remove(TfsFileSystem* fs, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath name;
	tfs_path_split_last(path, &parent_path, &name);

	// If we can't find the parent directory, return Err
	TfsInodeIdx parent_idx;
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	if (tfs_find(fs, parent_path, &parent_idx, &parent_type, &parent_data) != TfsFileSystemFindErrorSuccess) {
		return TfsFileSystemRemoveErrorInexistentParentDirectory;
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return TfsFileSystemRemoveErrorParentNotDir;
	}

	// If there isn't an entry with the name, return Err
	TfsInodeIdx idx;
	if (tfs_inode_dir_search_by_name(&parent_data->dir, name.chars, name.len, &idx) != TfsInodeDirErrorSuccess) {
		return TfsFileSystemRemoveErrorNameNotFound;
	}

	// Get the type and data of the node to delete
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_get(&fs->inode_table, idx, &type, &data) == TfsInodeTableGetErrorSuccess);

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		return TfsFileSystemRemoveErrorRemoveNonEmptyDir;
	}

	// Else remove it from the directory
	assert(tfs_inode_dir_remove_entry(&parent_data->dir, idx) == TfsInodeDirErrorSuccess);

	// And delete it from the inode table
	assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);

	return TfsFileSystemRemoveErrorSuccess;
}

TfsFileSystemFindError tfs_find(TfsFileSystem* fs, TfsPath path, TfsInodeIdx* idx, TfsInodeType* type, TfsInodeData** data) {
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
			return TfsFileSystemFindErrorSuccess;
		}

		// Get the current inode's type and data
		assert(tfs_inode_table_get(&fs->inode_table, cur_idx, &cur_type, &cur_data) == TfsInodeTableGetErrorSuccess);

		// Else, if this isn't a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			return TfsFileSystemFindErrorParentsNotDir;
		}

		// Get the name of the current inode we're in and skip it.
		TfsPath cur_path;
		tfs_path_split_first(path, &cur_path, &path);

		// Try to get the node
		if (tfs_inode_dir_search_by_name(&cur_data->dir, cur_path.chars, cur_path.len, &cur_idx) != TfsInodeDirErrorSuccess) {
			return TfsFileSystemFindErrorNameNotFound;
		}
	} while (1);
}

void tfs_print(TfsFileSystem* fs, FILE* out) {
	tfs_inode_table_print_tree(&fs->inode_table, out, 0, "");
}
