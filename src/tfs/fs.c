#include "fs.h"

// Includes
#include <assert.h> // assert
#include <string.h> // strcpy

void tfs_fs_find_result_print(const TfsFsFindResult* result, FILE* out) {
	switch (result->kind) {
		case TfsFsFindResultErrorParentsNotDir: {
			const TfsPath* path = &result->data.parents_not_dir.path;
			fprintf(out, "Entry '%.*s' is not a directory\n", path->len, path->chars);
			break;
		}

		case TfsFsFindResultErrorNameNotFound: {
			const TfsPath* entry_name = &result->data.name_not_found.entry_name;
			fprintf(out, "Entry '%.*s' does not exist\n", entry_name->len, entry_name->chars);
			break;
		}

		case TfsFsFindResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

void tfs_fs_create_result_print(const TfsFsCreateResult* result, FILE* out) {
	switch (result->kind) {
		case TfsFsCreateResultErrorInexistentParentDir: {
			const TfsPath* parent_path = &result->data.inexistent_parent_dir.parent;
			fprintf(out, "Unable to find parent directory '%.*s'\n", parent_path->len, parent_path->chars);
			tfs_fs_find_result_print(&result->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsCreateResultErrorParentNotDir: {
			const TfsPath* parent_path = &result->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsCreateResultErrorCreateInode:
			fprintf(out, "Unable to create inode\n");
			// TODO: Print underlying error
			break;

		case TfsFsCreateResultErrorAddEntry:
			fprintf(out, "Unable to add directory entry\n");
			tfs_inode_dir_add_entry_result_print(&result->data.add_entry.err, out);
			break;

		case TfsFsCreateResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

void tfs_fs_remove_result_print(const TfsFsRemoveResult* result, FILE* out) {
	switch (result->kind) {
		case TfsFsRemoveResultErrorInexistentParentDir: {
			const TfsPath* parent_path = &result->data.inexistent_parent_dir.parent;
			fprintf(out, "Unable to find parent directory '%.*s'\n", parent_path->len, parent_path->chars);
			tfs_fs_find_result_print(&result->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsRemoveResultErrorParentNotDir: {
			const TfsPath* parent_path = &result->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsRemoveResultErrorNameNotFound: {
			const TfsPath* entry_name = &result->data.name_not_found.entry_name;
			fprintf(out, "Cannot find entry '%.*s'\n", entry_name->len, entry_name->chars);
			break;
		}

		case TfsFsRemoveResultErrorRemoveNonEmptyDir: {
			const TfsPath* dir_name = &result->data.remove_non_empty_dir.dir_name;
			fprintf(out, "Cannot remove non-empty directory '%.*s'\n", dir_name->len, dir_name->chars);
			break;
		}

		case TfsFsRemoveResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

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

TfsFsCreateResult tfs_fs_create(TfsFs* fs, TfsInodeType type, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	TfsFsFindResult find_result = tfs_fs_find(fs, parent_path);
	if (find_result.kind != TfsFsFindResultSuccess) {
		return (TfsFsCreateResult){
			.kind = TfsFsCreateResultErrorInexistentParentDir,
			.data = {.inexistent_parent_dir = {.err = find_result, .parent = parent_path}}};
	}
	TfsInodeType parent_type  = find_result.data.success.type;
	TfsInodeData* parent_data = find_result.data.success.data;

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return (TfsFsCreateResult){.kind = TfsFsCreateResultErrorParentNotDir, .data = {.parent_not_dir = {.parent = parent_path}}};
	}

	// Else create the inode
	TfsInodeIdx idx;
	TfsInodeTableCreateError create_err = tfs_inode_table_create(&fs->inode_table, type, &idx, NULL);
	if (create_err != TfsInodeTableCreateErrorSuccess) {
		return (TfsFsCreateResult){.kind = TfsFsCreateResultErrorCreateInode, .data = {.create_inode = {.err = create_err}}};
	}

	// And add it to the directory
	TfsInodeDirAddEntryResult add_entry_res = tfs_inode_dir_add_entry(&parent_data->dir, idx, entry_name.chars, entry_name.len);
	if (add_entry_res.kind != TfsInodeDirAddEntryResultSuccess) {
		// Note: If unable to, we delete the inode we just created.
		assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);
		return (TfsFsCreateResult){.kind = TfsFsCreateResultErrorAddEntry, .data = {.add_entry = {.err = add_entry_res}}};
	}

	return (TfsFsCreateResult){.kind = TfsFsCreateResultSuccess};
}

TfsFsRemoveResult tfs_fs_remove(TfsFs* fs, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	TfsFsFindResult find_result = tfs_fs_find(fs, parent_path);
	if (find_result.kind != TfsFsFindResultSuccess) {
		return (TfsFsRemoveResult){
			.kind = TfsFsRemoveResultErrorInexistentParentDir,
			.data = {.inexistent_parent_dir = {.err = find_result, .parent = parent_path}}};
	}
	TfsInodeType parent_type  = find_result.data.success.type;
	TfsInodeData* parent_data = find_result.data.success.data;

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		return (TfsFsRemoveResult){.kind = TfsFsRemoveResultErrorParentNotDir, .data = {.parent_not_dir = {.parent = parent_path}}};
	}

	// If there isn't an entry with the name, return Err
	TfsInodeIdx idx = tfs_inode_dir_search_by_name(&parent_data->dir, entry_name.chars, entry_name.len);
	if (idx == TFS_INODE_IDX_NONE) {
		return (TfsFsRemoveResult){.kind = TfsFsRemoveResultErrorNameNotFound, .data = {.name_not_found = {.entry_name = entry_name}}};
	}

	// Get the type and data of the node to delete
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_get(&fs->inode_table, idx, &type, &data) == TfsInodeTableGetErrorSuccess);

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		return (TfsFsRemoveResult){.kind = TfsFsRemoveResultErrorRemoveNonEmptyDir, .data = {.remove_non_empty_dir = {.dir_name = entry_name}}};
	}

	// Else remove it from the directory
	assert(tfs_inode_dir_remove_entry(&parent_data->dir, idx));

	// And delete it from the inode table
	assert(tfs_inode_table_remove(&fs->inode_table, idx) == TfsInodeTableRemoveErrorSuccess);

	return (TfsFsRemoveResult){.kind = TfsFsRemoveResultSuccess};
}

TfsFsFindResult tfs_fs_find(TfsFs* fs, TfsPath path) {
	// Ignore leading slash on `path`
	if (path.len > 0 && path.chars[0] == '/') {
		path.chars++;
		path.len--;
	}

	// Current path and index
	TfsInodeIdx cur_idx = 0;
	TfsPath cur_path	= path;

	do {
		// Get the current inode's type and data
		TfsInodeData* cur_data;
		TfsInodeType cur_type;
		assert(tfs_inode_table_get(&fs->inode_table, cur_idx, &cur_type, &cur_data) == TfsInodeTableGetErrorSuccess);

		// If there's no more path to split, return the current inode
		if (cur_path.len == 0) {
			return (TfsFsFindResult){.kind = TfsFsFindResultSuccess, .data = {.success = {.idx = cur_idx, .type = cur_type, .data = cur_data}}};
		}

		// Get the name of the current inode we're in and skip it.
		TfsPath cur_dir;
		tfs_path_split_first(cur_path, &cur_dir, &cur_path);

		// Else, if this isn't a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = cur_dir.chars - path.chars;
			return (TfsFsFindResult){.kind = TfsFsFindResultErrorParentsNotDir, .data = {.parents_not_dir = {.path = bad_dir_path}}};
		}

		// Try to get the node
		cur_idx = tfs_inode_dir_search_by_name(&cur_data->dir, cur_dir.chars, cur_dir.len);
		if (cur_idx == TFS_INODE_IDX_NONE) {
			return (TfsFsFindResult){.kind = TfsFsFindResultErrorNameNotFound};
		}
	} while (1);
}

void tfs_fs_print(TfsFs* fs, FILE* out) {
	tfs_inode_table_print_tree(&fs->inode_table, out, 0, "");
}
