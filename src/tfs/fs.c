#include "fs.h"

// Includes
#include <assert.h> // assert
#include <string.h> // strcpy

/// @brief
/// Similarly to #tfs_fs_find , locks and retrives an inode's data
/// starting at @p start_idx . This node must already be locked for `access`.
/// @param self
/// @param path The path of the inode to get.
/// @param access Access type to lock te result with.
/// @param start_idx The inode to start at. _Must_ be locked.
/// @param start_type The start inode's type.
/// @param start_data The start inode's data.
/// @param unlock_start If the start inode should be unlocked or not.
/// @param[out] type The type of the inode.
/// @param[out] data The data of the inode.
/// @param[out] err Set if any errors occur.
/// @return Index of the inode, if found. Otherwise #TFS_INODE_IDX_NONE
/// @warning The returned inode _must_ be unlocked.
static TfsInodeIdx tfs_fs_find_from(
	TfsFs* self,
	TfsPath path,
	TfsRwLockAccess access,
	TfsInodeIdx start_idx,
	TfsInodeType start_type,
	TfsInodeData* start_data,
	bool unlock_start,
	TfsInodeType* type,
	TfsInodeData** data,
	TfsFsFindError* err);

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
			fprintf(out, "Unable to find parent directory\n");
			tfs_fs_find_error_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsCreateErrorParentNotDir: {
			fprintf(out, "Parent directory was not a directory\n");
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
			fprintf(out, "Unable to find parent directory\n");
			tfs_fs_find_error_print(&self->data.inexistent_parent_dir.err, out);
			break;
		}

		case TfsFsRemoveErrorParentNotDir: {
			fprintf(out, "Parent directory was not a directory\n");
			break;
		}

		case TfsFsRemoveErrorNameNotFound: {
			fprintf(out, "Cannot find entry in parent directory\n");
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

void tfs_fs_move_error_print(const TfsFsMoveError* self, FILE* out) {
	switch (self->kind) {
		case TfsFsMoveErrorInexistentCommonAncestor: {
			fprintf(out, "The common ancestor of both paths was not found\n");
			break;
		}
		case TfsFsMoveErrorOriginDestinationParent: {
			fprintf(out, "The origin path was the destination path's parent\n");
			break;
		}
		case TfsFsMoveErrorDestinationOriginParent: {
			fprintf(out, "The destination path was the origin's path parent\n");
			break;
		}
		case TfsFsMoveErrorInexistentOriginParentDir: {
			fprintf(out, "The origin path's parent did not exist\n");
			break;
		}
		case TfsFsMoveErrorInexistentDestinationParentDir: {
			fprintf(out, "The destination path's parent did not exist\n");
			break;
		}
		case TfsFsMoveErrorOriginParentNotDir: {
			fprintf(out, "The origin path's parent was not a directory\n");
			break;
		}
		case TfsFsMoveErrorDestinationParentNotDir: {
			fprintf(out, "The destination path's parent was not a directory\n");
			break;
		}
		case TfsFsMoveErrorOriginNotFound: {
			fprintf(out, "The origin path was not found\n");
			break;
		}
		case TfsFsMoveErrorAddEntry: {
			fprintf(out, "Unable to add an entry to the destination path's parent\n");
			tfs_inode_dir_add_entry_error_print(&self->data.add_entry.err, out);
			break;
		}
		default: {
			break;
		}
	}
}

TfsFs tfs_fs_new(void) {
	// Create the inode table
	TfsFs fs = {.inode_table = tfs_inode_table_new()};

	// Create the root node at index `0` and unlock it
	// SAFETY: We just created it, so we can unlock it
	TfsInodeIdx idx = tfs_inode_table_add(&fs.inode_table, TfsInodeTypeDir, TfsRwLockAccessUnique);
	assert(idx == 0);
	assert(tfs_inode_table_unlock_inode(&fs.inode_table, idx));

	return fs;
}

void tfs_fs_destroy(TfsFs* self) {
	// Destroy the inode table
	tfs_inode_table_destroy(&self->inode_table);
}

TfsInodeIdx tfs_fs_create(TfsFs* self, TfsPath path, TfsInodeType type, TfsFsCreateError* err) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name = tfs_path_pop_last(path, &parent_path);

	// Create the new inode
	// Note: We need to do this before finding the parent because the inode table requires
	//       unique access to add a new inode.
	TfsInodeIdx idx = tfs_inode_table_add(&self->inode_table, type, TfsRwLockAccessUnique);

	// Try to find the parent directory, if we can't, return Err.
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, TfsRwLockAccessUnique, &parent_type, &parent_data, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		// SAFETY: We locked the child inode when we created it
		assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
		if (err != NULL) {
			*err = (TfsFsCreateError){
				.kind = TfsFsCreateErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err}}};
		}
		return TFS_INODE_IDX_NONE;
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		// SAFETY: We locked the child inode when we created it
		assert(tfs_inode_table_remove_inode(&self->inode_table, idx));
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsCreateError){.kind = TfsFsCreateErrorParentNotDir};
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

bool tfs_fs_remove(TfsFs* self, TfsPath path, TfsFsRemoveError* err) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name = tfs_path_pop_last(path, &parent_path);

	// Try to find the parent directory, if we can't, return Err.
	TfsInodeType parent_type;
	TfsInodeData* parent_data;
	TfsFsFindError parent_find_err;
	TfsInodeIdx parent_idx = tfs_fs_find(self, parent_path, TfsRwLockAccessUnique, &parent_type, &parent_data, &parent_find_err);
	if (parent_idx == TFS_INODE_IDX_NONE) {
		if (err != NULL) {
			*err = (TfsFsRemoveError){
				.kind = TfsFsRemoveErrorInexistentParentDir,
				.data = {.inexistent_parent_dir = {.err = parent_find_err}}};
		}
		return false;
	}

	// If the parent isn't a directory, return Err
	if (parent_type != TfsInodeTypeDir) {
		// SAFETY: We locked the parent with `tfs_fs_find`.
		assert(tfs_inode_table_unlock_inode(&self->inode_table, parent_idx));
		if (err != NULL) {
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorParentNotDir};
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
			*err = (TfsFsRemoveError){.kind = TfsFsRemoveErrorNameNotFound};
		}
		return false;
	}

	// Lock the inode we're deleting with unique access to ensure no one else is using it.
	// SAFETY: As we have the parent locked, we guarantee `idx` will
	//         stay alive until we can lock it for deletion.
	TfsInodeType type;
	TfsInodeData* data;
	assert(tfs_inode_table_lock(&self->inode_table, idx, TfsRwLockAccessUnique, &type, &data));

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

TfsInodeIdx tfs_fs_find(TfsFs* self, TfsPath path, TfsRwLockAccess access, TfsInodeType* type, TfsInodeData** data, TfsFsFindError* err) {
	// Lock the root node for ourselves and unlock the given lock
	TfsInodeIdx start_idx = 0;
	TfsInodeData* start_data;
	TfsInodeType start_type;
	assert(tfs_inode_table_lock(&self->inode_table, start_idx, access, &start_type, &start_data));

	// Note: We unlock the root when calling this function
	return tfs_fs_find_from(self, path, access, start_idx, start_type, start_data, true, type, data, err);
}

TfsInodeIdx tfs_fs_move(TfsFs* self, TfsPath orig_path, TfsPath dest_path, TfsRwLockAccess access, TfsInodeType* const type, TfsInodeData** const data, TfsFsMoveError* const err) {
	// Get the common ancestor of both paths
	TfsPath orig_path_rest;
	TfsPath dest_path_rest;
	TfsPath common_ancestor = tfs_path_common_ancestor(orig_path, dest_path, &orig_path_rest, &dest_path_rest);

	// Lock the common ancestor
	TfsInodeType common_ancestor_type;
	TfsInodeData* common_ancestor_data;
	TfsFsFindError find_common_ancestor_err;
	TfsInodeIdx common_ancestor_idx = tfs_fs_find(self, common_ancestor, TfsRwLockAccessUnique, &common_ancestor_type, &common_ancestor_data, &find_common_ancestor_err);
	if (common_ancestor_idx == TFS_INODE_IDX_NONE) {
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorInexistentCommonAncestor,
				.data = {.inexistent_common_ancestor = {.err = find_common_ancestor_err}},
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Split the rests
	TfsPath orig_path_parent;
	TfsPath orig_path_filename = tfs_path_pop_last(orig_path_rest, &orig_path_parent);

	TfsPath dest_path_parent;
	TfsPath dest_path_filename = tfs_path_pop_last(dest_path_rest, &dest_path_parent);

	// Lock the origin parent
	TfsInodeType orig_parent_type;
	TfsInodeData* orig_parent_data;
	TfsFsFindError find_orig_parent_err;
	TfsInodeIdx orig_parent_idx = tfs_fs_find_from(self,
		orig_path_parent,
		TfsRwLockAccessUnique,
		common_ancestor_idx,
		common_ancestor_type,
		common_ancestor_data,
		false,
		&orig_parent_type,
		&orig_parent_data,
		&find_orig_parent_err);

	// If we couldn't lock it, return Err
	if (orig_parent_idx == TFS_INODE_IDX_NONE) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorInexistentOriginParentDir,
			};
		}
		return TFS_INODE_IDX_NONE;
	}
	// Check if the parent is the same as common ancestor.
	// Note: Even if they're equal, no deadlocks occur with `tfs_fs_find_from´,
	//       as it never locks the inode we give it.
	if (common_ancestor_idx == orig_parent_idx) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorDestinationOriginParent,
			};
		}
		return TFS_INODE_IDX_NONE;
	}
	// If the parent isn't a directory, return Err
	if (orig_parent_type != TfsInodeTypeDir) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorOriginParentNotDir,
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	TfsInodeType dest_parent_type;
	TfsInodeData* dest_parent_data;
	TfsFsFindError find_dest_parent_err;
	TfsInodeIdx dest_parent_idx = tfs_fs_find_from(self,
		dest_path_parent,
		TfsRwLockAccessUnique,
		common_ancestor_idx,
		common_ancestor_type,
		common_ancestor_data,
		false,
		&dest_parent_type,
		&dest_parent_data,
		&find_dest_parent_err);

	// If we couldn't lock it, return Err
	if (dest_parent_idx == TFS_INODE_IDX_NONE) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorInexistentDestinationParentDir,
			};
		}
		return TFS_INODE_IDX_NONE;
	}
	// Check if the parent is the same as common ancestor.
	// Note: Even if they're equal, no deadlocks occur with `tfs_fs_find_from´,
	//       as it never locks the inode we give it.
	if (common_ancestor_idx == dest_parent_idx) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorOriginDestinationParent,
			};
		}
		return TFS_INODE_IDX_NONE;
	}
	// If the parent isn't a directory, return Err
	if (dest_parent_type != TfsInodeTypeDir) {
		assert(tfs_fs_unlock_inode(self, common_ancestor_idx));
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		assert(tfs_fs_unlock_inode(self, dest_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorDestinationParentNotDir,
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Release the common ancestor
	assert(tfs_fs_unlock_inode(self, common_ancestor_idx));

	// Find the origin file
	size_t orig_path_dir_idx;
	TfsInodeIdx orig_path_idx = tfs_inode_dir_search_by_name(
		&orig_parent_data->dir,
		orig_path_filename.chars,
		orig_path_filename.len,
		&orig_path_dir_idx);
	if (orig_path_idx == TFS_INODE_IDX_NONE) {
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		assert(tfs_fs_unlock_inode(self, dest_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorOriginNotFound,
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Lock the origin file
	assert(tfs_inode_table_lock(&self->inode_table, orig_path_idx, access, type, data));

	// Add the file to the new directory
	TfsInodeDirAddEntryError add_entry_err;
	bool add_entry_success = tfs_inode_dir_add_entry(
		&dest_parent_data->dir,
		orig_path_idx,
		dest_path_filename.chars,
		dest_path_filename.len,
		&add_entry_err);
	if (!add_entry_success) {
		assert(tfs_fs_unlock_inode(self, orig_parent_idx));
		assert(tfs_fs_unlock_inode(self, dest_parent_idx));
		if (err != NULL) {
			*err = (TfsFsMoveError){
				.kind = TfsFsMoveErrorAddEntry,
				.data = {.add_entry = {.err = add_entry_err}},
			};
		}
		return TFS_INODE_IDX_NONE;
	}

	// Release destination's parent lock
	assert(tfs_fs_unlock_inode(self, dest_parent_idx));

	// Remove the source entry
	assert(tfs_inode_dir_remove_entry_by_dir_idx(&orig_parent_data->dir, orig_path_dir_idx));

	// Remove the origin's parent lock
	assert(tfs_fs_unlock_inode(self, orig_parent_idx));

	// Return the index
	// Note: We returnede it locked.
	return orig_path_idx;
}

bool tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx) {
	// Simply delegate to the inode table
	return tfs_inode_table_unlock_inode(&self->inode_table, idx);
}

void tfs_fs_print(const TfsFs* self, FILE* out) {
	// Print the root inode and all it's children
	// Note: We start off with '' as the root, instead of '/'.
	tfs_inode_table_print_tree(&self->inode_table, 0, out, "");
}

static TfsInodeIdx tfs_fs_find_from(
	TfsFs* self,
	TfsPath path,
	TfsRwLockAccess access,
	TfsInodeIdx start_idx,
	TfsInodeType start_type,
	TfsInodeData* start_data,
	bool unlock_start,
	TfsInodeType* type,
	TfsInodeData** data,
	TfsFsFindError* err //
) {
	TfsInodeIdx cur_idx	   = start_idx;
	TfsInodeType cur_type  = start_type;
	TfsInodeData* cur_data = start_data;

	// The current index we're checking
	TfsPath cur_path = tfs_path_trim(path);

	do {
		// If there's no more path to split, set the type and data and return the current inode
		// Note: We leave `cur_idx` locked for when we return.
		if (cur_path.len == 0) {
			if (type != NULL) { *type = cur_type; }
			if (data != NULL) { *data = cur_data; }
			return cur_idx;
		}

		// Get the name of the current inode we're in and set
		// the current path to the remaining children.
		TfsPath cur_dir = tfs_path_pop_first(cur_path, &cur_path);

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
		assert(tfs_inode_table_lock(&self->inode_table, child_idx, access, &cur_type, &cur_data));
		// Note: If the user told us to not unlock the start inode, skip it.
		if ((unlock_start && cur_idx == start_idx) || (cur_idx != start_idx)) {
			assert(tfs_inode_table_unlock_inode(&self->inode_table, cur_idx));
		}
		cur_idx = child_idx;
	} while (1);
}
