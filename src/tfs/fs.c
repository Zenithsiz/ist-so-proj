#include "fs.h"

// Includes
#include <assert.h> // assert
#include <string.h> // strcpy

void tfs_fs_find_result_print(const TfsFsFindResult* self, FILE* out) {
	switch (self->kind) {
		case TfsFsFindResultErrorParentsNotDir: {
			const TfsPath* path = &self->data.parents_not_dir.path;
			fprintf(out, "Entry '%.*s' is not a directory\n", (int)path->len, path->chars);
			break;
		}

		case TfsFsFindResultErrorNameNotFound: {
			const TfsPath* path = &self->data.name_not_found.path;
			fprintf(out, "Entry '%.*s' does not exist\n", (int)path->len, path->chars);
			break;
		}

		case TfsFsFindResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

void tfs_fs_create_result_print(const TfsFsCreateResult* self, FILE* out) {
	switch (self->kind) {
		case TfsFsCreateResultErrorInexistentParentDir: {
			const TfsPath* parent_path = &self->data.inexistent_parent_dir.parent;
			fprintf(out, "Unable to find parent directory '%.*s'\n", (int)parent_path->len, parent_path->chars);
			tfs_fs_find_result_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsCreateResultErrorParentNotDir: {
			const TfsPath* parent_path = &self->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", (int)parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsCreateResultErrorAddEntry:
			fprintf(out, "Unable to add directory entry\n");
			tfs_inode_dir_add_entry_result_print(&self->data.add_entry.err, out);
			break;

		case TfsFsCreateResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

void tfs_fs_remove_result_print(const TfsFsRemoveResult* self, FILE* out) {
	switch (self->kind) {
		case TfsFsRemoveResultErrorInexistentParentDir: {
			const TfsPath* parent_path = &self->data.inexistent_parent_dir.parent;
			fprintf(out, "Unable to find parent directory '%.*s'\n", (int)parent_path->len, parent_path->chars);
			tfs_fs_find_result_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsRemoveResultErrorParentNotDir: {
			const TfsPath* parent_path = &self->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", (int)parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsRemoveResultErrorNameNotFound: {
			const TfsPath* entry_name = &self->data.name_not_found.entry_name;
			fprintf(out, "Cannot find entry '%.*s'\n", (int)entry_name->len, entry_name->chars);
			break;
		}

		case TfsFsRemoveResultErrorRemoveNonEmptyDir: {
			const TfsPath* dir_name = &self->data.remove_non_empty_dir.dir_name;
			fprintf(out, "Cannot remove non-empty directory '%.*s'\n", (int)dir_name->len, dir_name->chars);
			break;
		}

		case TfsFsRemoveResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

TfsFs tfs_fs_new(void) {
	// Create the inode table
	TfsFs fs = {.inode_table = tfs_inode_table_new()};

	// Create the root node at index `0`.
	TfsInodeIdx idx;
	tfs_inode_table_create(&fs.inode_table, TfsInodeTypeDir, &idx, NULL);
	if (idx != 0) {
		fprintf(stderr, "Failed to create root");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void tfs_fs_destroy(TfsFs* self) {
	// Destroy the inode table
	tfs_inode_table_destroy(&self->inode_table);
}

TfsFsCreateResult tfs_fs_create(TfsFs* self, TfsInodeType type, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	TfsFsFindResult parent_find_result = tfs_fs_find(self, parent_path);
	if (parent_find_result.kind != TfsFsFindResultSuccess) {
		return (TfsFsCreateResult){
			.kind = TfsFsCreateResultErrorInexistentParentDir,
			.data = {.inexistent_parent_dir = {.err = parent_find_result, .parent = parent_path}}};
	}

	// If the parent isn't a directory, return Err
	if (parent_find_result.data.success.type != TfsInodeTypeDir) {
		return (TfsFsCreateResult){.kind = TfsFsCreateResultErrorParentNotDir, .data = {.parent_not_dir = {.parent = parent_path}}};
	}

	// Else create the inode
	TfsInodeIdx idx;
	tfs_inode_table_create(&self->inode_table, type, &idx, NULL);

	// Get the parent's data
	// Note: We only do this here, instead of at the previous `find` because
	//       it's possible that `tfs_inode_table_create` invalidates pointers
	//       when it expands the inode table.
	TfsInodeData* parent_data;
	assert(tfs_inode_table_get(&self->inode_table, parent_find_result.data.success.idx, NULL, &parent_data));

	// And add it to the directory
	TfsInodeDirAddEntryResult add_entry_res = tfs_inode_dir_add_entry(&parent_data->dir, idx, entry_name.chars, entry_name.len);
	if (add_entry_res.kind != TfsInodeDirAddEntryResultSuccess) {
		// Note: If unable to, we delete the inode we just created.
		assert(tfs_inode_table_remove(&self->inode_table, idx));
		return (TfsFsCreateResult){.kind = TfsFsCreateResultErrorAddEntry, .data = {.add_entry = {.err = add_entry_res}}};
	}

	return (TfsFsCreateResult){.kind = TfsFsCreateResultSuccess, .data = {.success = {.idx = idx}}};
}

TfsFsRemoveResult tfs_fs_remove(TfsFs* self, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	TfsFsFindResult find_result = tfs_fs_find(self, parent_path);
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
	assert(tfs_inode_table_get(&self->inode_table, idx, &type, &data));

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		return (TfsFsRemoveResult){.kind = TfsFsRemoveResultErrorRemoveNonEmptyDir, .data = {.remove_non_empty_dir = {.dir_name = entry_name}}};
	}

	// Else remove it from the directory
	assert(tfs_inode_dir_remove_entry(&parent_data->dir, idx));

	// And delete it from the inode table
	assert(tfs_inode_table_remove(&self->inode_table, idx));

	return (TfsFsRemoveResult){.kind = TfsFsRemoveResultSuccess};
}

TfsFsFindResult tfs_fs_find(TfsFs* self, TfsPath path) {
	// Ignore leading slash on `path`
	if (path.len > 0 && path.chars[0] == '/') {
		path.chars++;
		path.len--;
	}

	// Current path and index
	TfsInodeIdx cur_idx = 0;
	TfsPath cur_path	= path;
	TfsPath cur_dir		= tfs_path_from_cstr("");

	do {
		// Get the current inode's type and data
		TfsInodeData* cur_data;
		TfsInodeType cur_type;
		assert(tfs_inode_table_get(&self->inode_table, cur_idx, &cur_type, &cur_data));

		// If there's no more path to split, return the current inode
		if (cur_path.len == 0) {
			return (TfsFsFindResult){.kind = TfsFsFindResultSuccess, .data = {.success = {.idx = cur_idx, .type = cur_type, .data = cur_data}}};
		}

		// Else, if this isn't a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars);
			return (TfsFsFindResult){.kind = TfsFsFindResultErrorParentsNotDir, .data = {.parents_not_dir = {.path = bad_dir_path}}};
		}

		// Get the name of the current inode we're in and skip it.
		tfs_path_split_first(cur_path, &cur_dir, &cur_path);

		// Try to get the node
		cur_idx = tfs_inode_dir_search_by_name(&cur_data->dir, cur_dir.chars, cur_dir.len);
		if (cur_idx == TFS_INODE_IDX_NONE) {
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars);
			return (TfsFsFindResult){.kind = TfsFsFindResultErrorNameNotFound, .data = {.name_not_found = {.path = bad_dir_path}}};
		}
	} while (1);
}

void tfs_fs_print(TfsFs* self, FILE* out) {
	tfs_inode_table_print_tree(&self->inode_table, out, 0, "");
}
