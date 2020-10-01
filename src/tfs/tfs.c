#include "tfs.h"

// Includes
#include <stdio.h>	// stderr, fprintf
#include <string.h> // strcpy

TfsFileSystem tfs_new(size_t max_inodes) {
	// Create the inode table
	TfsFileSystem fs = {.inode_table = tfs_inode_table_new(max_inodes)};

	// Create root node
	TfsInodeIdx root;
	if (tfs_inode_table_create(&fs.inode_table, TfsInodeTypeDir, &root, NULL) != TfsInodeTableErrorSuccess || root != 0) {
		fprintf(stderr, "Failed to create root");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void tfs_drop(TfsFileSystem* fs) {
	// Drop the inode table
	tfs_inode_table_drop(&fs->inode_table);
}

TfsFileSystemError tfs_create_inode(TfsFileSystem* fs, TfsInodeType type, TfsPath path) {
	TfsInodeIdx	 parent_inumber, child_inumber;
	TfsPath		 parent_name;
	TfsPath		 child_name;
	TfsInodeType pType;
	TfsInodeData p_data;

	tfs_path_split_last(path, &parent_name, &child_name);

	if (tfs_find(fs, parent_name, &parent_inumber) != TfsFileSystemErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_dir_search_by_name(&p_data.dir, child_name.chars, child_name.len, NULL) == TfsInodeDirErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	/* create node and add entry to folder that contains new node */
	if (tfs_inode_table_create(&fs->inode_table, type, &child_inumber, NULL) != TfsInodeTableErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_dir_add_entry(&p_data.dir, child_inumber, child_name.chars, child_name.len) != TfsInodeDirErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	return TfsFileSystemErrorSuccess;
}

TfsFileSystemError tfs_delete_inode(TfsFileSystem* fs, TfsPath path) {
	TfsInodeIdx	 parent_inumber, child_inumber;
	TfsPath		 parent_name;
	TfsPath		 child_name;
	TfsInodeType pType, cType;
	TfsInodeData p_data, cdata;

	tfs_path_split_last(path, &parent_name, &child_name);

	if (tfs_find(fs, parent_name, &parent_inumber) != TfsFileSystemErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, parent_inumber, &pType, &p_data);

	if (pType != TfsInodeTypeDir) {
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_dir_search_by_name(&p_data.dir, child_name.chars, child_name.len, &child_inumber) != TfsInodeDirErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	tfs_inode_table_get(&fs->inode_table, child_inumber, &cType, &cdata);

	if (cType == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&cdata.dir)) {
		return TfsFileSystemErrorOther;
	}

	/* remove entry from folder that contained deleted node */
	if (tfs_inode_dir_remove_entry(&p_data.dir, child_inumber) != TfsInodeDirErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	if (tfs_inode_table_delete(&fs->inode_table, child_inumber) != TfsInodeTableErrorSuccess) {
		return TfsFileSystemErrorOther;
	}

	return TfsFileSystemErrorSuccess;
}

TfsFileSystemError tfs_find(TfsFileSystem* fs, TfsPath path, TfsInodeIdx* idx) {
	// Current inode index and data
	TfsInodeIdx	 cur_idx = 0;
	TfsInodeData cur_inode_data;

	do {
		// Try to get the current inode data
		TfsInodeType type;
		if (tfs_inode_table_get(&fs->inode_table, cur_idx, &type, &cur_inode_data) != TfsInodeTableErrorSuccess) {
			return TfsFileSystemErrorOther;
		}

		// If there's no more path to split, return this inode
		// TODO: Switch order with above
		if (path.len == 0) {
			if (idx != NULL) {
				*idx = cur_idx;
			}
			return TfsFileSystemErrorSuccess;
		}

		// Else, if this isn't a directory, return Err
		if (type != TfsInodeTypeDir) {
			return TfsFileSystemErrorOther;
		}

		// Get the name of the current inode we're in and skip it.
		TfsPath cur_path;
		tfs_path_split_first(path, &cur_path, &path);

		// Try to get the node
		if (tfs_inode_dir_search_by_name(&cur_inode_data.dir, cur_path.chars, cur_path.len, &cur_idx) != TfsInodeDirErrorSuccess) {
			return TfsFileSystemErrorOther;
		}
	} while (1);
}

void tfs_print(TfsFileSystem* fs, FILE* out) {
	tfs_inode_table_print_tree(&fs->inode_table, out, 0, "");
}
