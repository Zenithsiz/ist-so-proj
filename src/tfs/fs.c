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

	// Create the root node at index `0` and unlock it
	// SAFETY: We just created it, so we can unlock it
	TfsInodeIdx idx = tfs_inode_table_add(&fs.inode_table, TfsInodeTypeDir, TfsLockAccessUnique);
	assert(idx == 0);
	assert(tfs_inode_table_unlock_inode(&fs.inode_table, idx));

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

	// Create the new inode
	// Note: We need to do this before finding the parent because the inode table requires
	//       unique access to add a new inode.
	TfsInodeIdx idx = tfs_inode_table_add(&self->inode_table, type, TfsLockAccessUnique);

	// Try to find the parent directory, if we can't, return Err.
	// Note: We don't pass `lock` to unlock here, since if this fails, we'll need
	//       to remove the newly created inode, which requires that we have
	//       unique ownership to the inode table.
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, NULL, TfsLockAccessUnique, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		// SAFETY: We locked the child inode when we created it
		// Note: `tfs_fs_find` unlocks `lock` even on error
		assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
		if (lock != NULL) { tfs_lock_unlock(lock); }
		if (err != NULL) {
			*err = (TfsFsCreateError){
				.kind = TfsFsCreateErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err, .parent = parent_path}}};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Unlock the filesystem lock, since we have the parent locked
	if (lock != NULL) { tfs_lock_unlock(lock); }

	// If the parent isn't a directory, return Err
	// SAFETY: `tfs_fs_find` locked the parent for unique access.
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	assert(tfs_inode_table_get(&self->inode_table, parent_idx, &parent_type, &parent_data));
	if (parent_type != TfsInodeTypeDir) {
		// SAFETY: We locked the child inode when we created it
		assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsCreateError){
				.kind = TfsFsCreateErrorParentNotDir,
				.data = {.parent_not_dir = {.parent = parent_path}},
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Else try to add it to the directory, if we can't, return Err.
	TfsInodeDirAddEntryError add_entry_err;
	if (!tfs_inode_dir_add_entry(&parent_data->dir, idx, entry_name.chars, entry_name.len, &add_entry_err)) {
		// SAFETY: We locked the child inode when we created it
		assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsCreateError){.kind = TfsFsCreateErrorAddEntry, .data = {.add_entry = {.err = add_entry_err}}};
		}
		return TFS_INODE_IDX_NONE;
	}

	// If we added it, unlock the parent and return the new inode locked.
	// SAFETY: We locked the parent with `tfs_fs_find`.
	assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
	return idx;
}

bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsLock* lock, TfsFsRemoveError* err) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name;
	tfs_path_split_last(path, &parent_path, &entry_name);

	// Try to find the parent directory, if we can't, return Err.
	// Note: As soon as we get the parent node, we unlock `lock`, as all we
	//       need to delete the file is the parent's lock (and the child's,
	//       but we can lock it later without ownership of `lock`).
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, lock, TfsLockAccessUnique, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		if (err != NULL) {
			*err = (TfsFsRemoveError){
				.kind = TfsFsRemoveErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err, .parent = parent_path}}};
		}
		return false;
	}

	// If the parent isn't a directory, return Err
	// SAFETY: `tfs_fs_find` locks the node for us with unique access.
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	assert(tfs_inode_table_get(&self->inode_table, parent_idx, &parent_type, &parent_data));
	if (parent_type != TfsInodeTypeDir) {
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsRemoveError){
				.kind = TfsFsRemoveErrorParentNotDir,
				.data = {.parent_not_dir = {.parent = parent_path}},
			};
		}
		return false;
	}

	// Try to find the inode index we need to delete, if we can't, return Err.
	size_t dir_idx;
	TfsInodeIdx idx = tfs_inode_dir_search_by_name(&parent_data->dir, entry_name.chars, entry_name.len, &dir_idx);
	if (idx == TFS_INODE_IDX_NONE) {
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorNameNotFound, .data = {.name_not_found = {.entry_name = entry_name}}};
		}
		return false;
	}

	// Lock the inode we're deleting with unique access to ensure no one else is using it.
	// SAFETY: As we have the parent locked, we guarantee `idx` will
	//         stay alive until we can lock it for deletion.
	assert(tfs_inode_table_lock(&self->inode_table, idx, TfsLockAccessUnique));

	// Get the type and data of the inode to delete
	// SAFETY: We just locked the inode.
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_get(&self->inode_table, idx, &type, &data));

	// If it's a directory but it's not empty, return Err
	if (type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&data->dir)) {
		// SAFETY: We just locked it.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, idx));
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorRemoveNonEmptyDir};
		}
		return false;
	}

	// Else remove it from the directory
	// SAFETY: We got `dir_idx` from `search_by_name`.
	assert(tfs_inode_dir_remove_entry_by_dir_idx(&parent_data->dir, dir_idx));

	// Remove it from the table and unlock the parent.
	// Note: This also unlocks it
	// SAFETY: We have the inode locked.
	assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
	// SAFETY: We locked the parent with `tfs_fs_find`.
	assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));

	return true;
}

TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsLock* lock, TfsLockAccess access, TfsFsFindError* err) {
	// Trim `path`
	tfs_path_trim(&path);

	// The current index we're checking
	TfsInodeIdx cur_idx = 0;
	TfsPath cur_path	= path;

	// Lock the root node for ourselves and unlock the given lock
	// Note: We can unlock here, as any next command will have to
	//       also lock the root node, and will have to wait until
	//       we're finished with it and it's children.
	tfs_inode_table_lock(&self->inode_table, cur_idx, access);
	if (lock != NULL) { tfs_lock_unlock(lock); }

	do {
		// If there's no more path to split, return the current inode
		// Note: We leave `cur_idx` locked for when we return.
		if (cur_path.len == 0) {
			return cur_idx;
		}

		// Get the current inode's type and data
		// SAFETY: We have the inode with index `cur_idx` locked.
		TfsInodeData* cur_data;
		TfsInodeType cur_type;
		assert(tfs_inode_table_get(&self->inode_table, cur_idx, &cur_type, &cur_data));

		// Get the name of the current inode we're in and set
		// the current path to the remaining children.
		TfsPath cur_dir;
		tfs_path_split_first(cur_path, &cur_dir, &cur_path);

		// If we're not a directory, return Err
		if (cur_type != TfsInodeTypeDir) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars);
			if (err != NULL) {
				*err = (TfsFsFindError){.kind = TfsFsFindErrorParentsNotDir, .data = {.parents_not_dir = {.path = bad_dir_path}}};
			}
			return TFS_INODE_IDX_NONE;
		}

		// Else try to get the child node's index
		// Note: If we can't find it, we just return Err.
		TfsInodeIdx child_idx = tfs_inode_dir_search_by_name(&cur_data->dir, cur_dir.chars, cur_dir.len, NULL);
		if (child_idx == TFS_INODE_IDX_NONE) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
			TfsPath bad_dir_path = path;
			bad_dir_path.len	 = (size_t)(cur_dir.chars - path.chars) + cur_dir.len;
			if (err != NULL) {
				*err = (TfsFsFindError){.kind = TfsFsFindErrorNameNotFound, .data = {.name_not_found = {.path = bad_dir_path}}};
			}
			return TFS_INODE_IDX_NONE;
		}

		// If we found it, lock it, unlock the parent node and start over
		// with the child node.
		assert(tfs_inode_table_lock(&self->inode_table, child_idx, access));
		assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
		cur_idx = child_idx;
	} while (1);
}

bool tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx) {
	// Simply delegate to the inode table
	return tfs_inode_table_unlock_inode(&self->inode_table, idx);
}

bool tfs_fs_get_inode(TfsFs* self, TfsInodeIdx idx, TfsInodeType* type, TfsInodeData** data) {
	// Simply delegate to the inode table
	// SAFETY: Caller ensures all safety requirements for this function.
	return tfs_inode_table_get(&self->inode_table, idx, type, data);
}

void tfs_fs_print(const TfsFs* self, FILE* out) {
	// Print the root inode and all it's children
	// Note: We start off with '' as the root, instead of '/'.
	tfs_inode_table_print_tree(&self->inode_table, 0, out, "");
}
