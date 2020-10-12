#include "fs.h"

// Includes
#include <assert.h> // assert
#include <string.h> // strcpy

void tfs_fs_find_error_print(const TfsFsFindError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsFindErrorParentsNotDir: {
			const TfsPath* path = &self->data.parents_not_dir.path;
			fprintf(out, "Entry '%.*s' is not a directory\n", (int)path->len, path->chars);
			break;
		}

		case TfsFsFindErrorNameNotFound: {
			const TfsPath* path = &self->data.name_not_found.path;
			fprintf(out, "Entry '%.*s' does not exist\n", (int)path->len, path->chars);
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_fs_create_error_print(const TfsFsCreateError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsCreateErrorInexistentParentDir: {
			const TfsPath* parent_path	   = &self->data.inexistent_parent_dir.parent;
			const TfsFsFindError* find_err = &self->data.inexistent_parent_dir.err;
			fprintf(out, "Unable to find parent directory '%.*s'\n", (int)parent_path->len, parent_path->chars);
			tfs_fs_find_error_print(find_err, out);
			break;
		}

		case TfsFsCreateErrorParentNotDir: {
			const TfsPath* parent_path = &self->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", (int)parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsCreateErrorAddEntry: {
			fprintf(out, "Unable to add directory entry\n");
			tfs_inode_dir_add_entry_error_print(&self->data.add_entry.err, out);
			break;
		}

		default: {
			break;
		}
	}
}

void tfs_fs_remove_error_print(const TfsFsRemoveError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsRemoveErrorInexistentParentDir: {
			const TfsPath* parent_path	   = &self->data.inexistent_parent_dir.parent;
			const TfsFsFindError* find_err = &self->data.inexistent_parent_dir.err;
			fprintf(out, "Unable to find parent directory '%.*s'\n", (int)parent_path->len, parent_path->chars);
			tfs_fs_find_error_print(find_err, out);
			break;
		}

		case TfsFsRemoveErrorParentNotDir: {
			const TfsPath* parent_path = &self->data.parent_not_dir.parent;
			fprintf(out, "Parent directory '%.*s' was not a directory\n", (int)parent_path->len, parent_path->chars);
			break;
		}

		case TfsFsRemoveErrorNameNotFound: {
			const TfsPath* entry_name = &self->data.name_not_found.entry_name;
			fprintf(out, "Cannot find entry '%.*s'\n", (int)entry_name->len, entry_name->chars);
			break;
		}

		case TfsFsRemoveErrorRemoveNonEmptyDir: {
			fprintf(out, "Directory was not empty\n");
			break;
		}

		default: {
			break;
		}
	}
}

TfsFs tfs_fs_new(TfsLockKind lock_kind) {
	// Create the inode table
	TfsFs fs = {.inode_table = tfs_inode_table_new(lock_kind)};

	// Create the root node at index `0`.
	TfsInodeIdx idx = tfs_inode_table_add(&fs.inode_table, TfsInodeTypeDir);
	assert(idx == 0);

	return fs;
}

void tfs_fs_destroy(TfsFs* self) {
	// Destroy the inode table
	tfs_inode_table_destroy(&self->inode_table);
}

TfsInodeIdx tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type, TfsLock* lock, TfsFsCreateError* err) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	// Note: We don't want to let go of `lock` yet, we need to write lock the parent inode.
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, NULL, TfsLockAccessUnique, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsCreateError){
				.kind = TfsFsCreateErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err, .parent = parent_path}}};
		}
		return TFS_INODE_IDX_NONE;
	}

	// If the parent isn't a directory, return Err
	TfsInodeType parent_type;
	assert(tfs_inode_table_get(&self->inode_table, parent_idx, &parent_type, NULL));
	if (parent_type != TfsInodeTypeDir) {
		tfs_inode_table_unlock_inode(&self->inode_table, parent_idx);
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsCreateError){
				.kind = TfsFsCreateErrorParentNotDir,
				.data = {.parent_not_dir = {.parent = parent_path}},
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Else create the inode
	TfsInodeIdx idx = tfs_inode_table_add(&self->inode_table, type);

	// Get the parent's data
	// Note: We get it here because `tfs_inode_table_create` invalidates
	//       pointers.
	TfsInodeData* parent_data;
	assert(tfs_inode_table_get(&self->inode_table, parent_idx, NULL, &parent_data));

	// Try to add it to the directory
	// Note: If unable to, we delete the inode we just created.
	TfsInodeDirAddEntryError add_entry_err;
	if (!tfs_inode_dir_add_entry(&parent_data->dir, idx, entry_name.chars, entry_name.len, &add_entry_err)) {
		assert(tfs_inode_table_remove(&self->inode_table, idx));
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsCreateError){.kind = TfsFsCreateErrorAddEntry, .data = {.add_entry = {.err = add_entry_err}}};
		}
		return TFS_INODE_IDX_NONE;
	}

	assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
	if (lock != NULL) { tfs_lock_unlock(lock); }
	return idx;
}

bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsLock* lock, TfsFsRemoveError* err) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// If we can't find the parent directory, return Err
	// Note: We don't want to let go of `lock` yet, we need to write lock the parent inode.
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, NULL, TfsLockAccessUnique, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsRemoveError){
				.kind = TfsFsRemoveErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err, .parent = parent_path}}};
		}
		return false;
	}

	// If the parent isn't a directory, return Err
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	assert(tfs_inode_table_get(&self->inode_table, parent_idx, &parent_type, &parent_data));
	if (parent_type != TfsInodeTypeDir) {
		tfs_inode_table_unlock_inode(&self->inode_table, parent_idx);
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsRemoveError){
				.kind = TfsFsRemoveErrorParentNotDir,
				.data = {.parent_not_dir = {.parent = parent_path}},
			};
		}
		return false;
	}

	// If there isn't an entry with the name, return Err
	TfsInodeIdx idx = tfs_inode_dir_search_by_name(&parent_data->dir, entry_name.chars, entry_name.len);
	if (idx == TFS_INODE_IDX_NONE) {
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorNameNotFound, .data = {.name_not_found = {.entry_name = entry_name}}};
		}
		return false;
	}

	// Get the type and data of the node to delete
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_get(&self->inode_table, idx, &type, &data));

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorRemoveNonEmptyDir};
		}
		return false;
	}

	// Else remove it from the directory
	assert(tfs_inode_dir_remove_entry(&parent_data->dir, idx));

	// Unlock our lock
	if (lock != NULL) { tfs_lock_unlock(lock); }

	// And delete it from the inode table
	assert(tfs_inode_table_remove(&self->inode_table, idx));

	return true;
}

TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsLock* lock, TfsLockAccess access, TfsFsFindError* err) {
	// Trim `path`
	tfs_path_trim(&path);

	// The current index we're checking
	TfsInodeIdx cur_idx = 0;
	TfsPath cur_path	= path;

	// Lock the root node for ourselves and unlock the given lock
	tfs_inode_table_lock(&self->inode_table, cur_idx, access);
	if (lock != NULL) { tfs_lock_unlock(lock); }

	do {
		// Get the current inode's type and data
		// SAFETY: We have the inode with index `cur_idx` locked.
		TfsInodeData* cur_data;
		TfsInodeType cur_type;
		assert(tfs_inode_table_get(&self->inode_table, cur_idx, &cur_type, &cur_data));

		// If there's no more path to split, return the current inode
		// Note: We leave `cur_idx` locked for when we return.
		if (cur_path.len == 0) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
			return cur_idx;
		}

		// Get the name of the current inode we're in and skip it.
		TfsPath cur_dir;
		tfs_path_split_first(cur_path, &cur_dir, &cur_path);

		// Else, if this isn't a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars);
			if (err != NULL) {
				*err = (TfsFsFindError){.kind = TfsFsFindErrorParentsNotDir, .data = {.parents_not_dir = {.path = bad_dir_path}}};
			}
			return TFS_INODE_IDX_NONE;
		}

		// Get the child node
		TfsInodeIdx child_idx = tfs_inode_dir_search_by_name(&cur_data->dir, cur_dir.chars, cur_dir.len);
		if (child_idx == TFS_INODE_IDX_NONE) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars);
			if (err != NULL) {
				*err = (TfsFsFindError){.kind = TfsFsFindErrorNameNotFound, .data = {.name_not_found = {.path = bad_dir_path}}};
			}
			return TFS_INODE_IDX_NONE;
		}

		// Lock it, unlock the parent node and set it as the current node.
		assert(tfs_inode_table_lock(&self->inode_table, child_idx, access));
		assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
		cur_idx = child_idx;
	} while (1);
}

void tfs_fs_print(const TfsFs* self, FILE* out) {
	// Print the root inode and all it's children
	tfs_inode_table_print_tree(&self->inode_table, 0, out, "");
}
