#include "fs.h"

// Includes
#include <assert.h>	  // assert
#include <string.h>	  // strcpy
#include <tfs/util.h> // tfs_str_cmp

/// @brief Helper function to lock all inodes until a given directory starting from a locked inode.
/// @param self
/// @param path The path to lock (all components except the last will be locked for reading).
/// @param start_inode The inode to start at. _Must_ be locked.
/// @param locked_inodes Array to store all locked inodes. Must be able to store as many components as exist in `path`.
/// @param access Type of access to lock the last component with
/// @details
/// Save all further inodes found (_not_ including the start inode) in @p locked_inodes
static TfsFsFindResult tfs_fs_lock_all_from(TfsFs* const self,
	TfsPath path,
	TfsLockedInode start_inode,
	TfsLockedInode* locked_inodes,
	TfsRwLockAccess access //
) {
	TfsPath cur_path = tfs_path_trim(path);

	TfsLockedInode cur_inode = start_inode;
	size_t locked_inodes_len = 0;

	while (cur_path.len != 0) {
		// Get the next component
		TfsPath cur_dir = tfs_path_pop_first(cur_path, &cur_path);

		// If we're not a directory, return Err
		if (cur_inode.type != TfsInodeTypeDir) {
			// Unlock all inodes locked so far
			for (size_t n = 0; n < locked_inodes_len; n++) {
				tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
			}

			// And return Err
			TfsPath bad_dir_path = path;
			bad_dir_path.len = (size_t)(cur_dir.chars - path.chars);
			return (TfsFsFindResult){
				.success = false,
				.data.err.kind = TfsFsFindErrorParentsNotDir,
				.data.err.data.parents_not_dir.path = bad_dir_path,
			};
		}

		// Else try to get the child node's index
		TfsInodeDirSearchByNameResult find_child_result =
			tfs_inode_dir_search_by_name(&cur_inode.data->dir, cur_dir.chars, cur_dir.len);
		if (!find_child_result.success) {
			// Unlock all inodes locked so far
			for (size_t n = 0; n < locked_inodes_len; n++) {
				tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
			}

			// And return Err
			TfsPath bad_dir_path = path;
			bad_dir_path.len = (size_t)(cur_dir.chars - path.chars) + cur_dir.len;
			return (TfsFsFindResult){
				.success = false,
				.data.err.kind = TfsFsFindErrorNameNotFound,
				.data.err.data.name_not_found.path = bad_dir_path,
			};
		}

		// If we found it, lock it and add it to the table
		TfsInodeIdx child_idx = find_child_result.data.success.idx;
		cur_inode =
			tfs_inode_table_lock(&self->inode_table, child_idx, cur_path.len == 0 ? access : TfsRwLockAccessShared);
		locked_inodes[locked_inodes_len] = cur_inode;
		locked_inodes_len++;
	}

	return (TfsFsFindResult){.success = true, .data.inode = locked_inodes[locked_inodes_len - 1]};
}

/// @brief Helper function to lock all inodes until a given directory starting from the root (while unlocked)
/// @param self.
/// @param path The path to lock (all components except the last will be locked for reading).
/// @param locked_inodes Array to store all locked inodes. Must be able to store as many components as exist in `path`
/// plus 1 for the root.
/// @param access Type of access to lock the last component with
/// @details
/// Saves the root _and_ all further components in @p locked_inodes
static TfsFsFindResult tfs_fs_lock_all(TfsFs* const self,
	TfsPath path,
	TfsLockedInode* locked_inodes,
	TfsRwLockAccess access //
) {
	locked_inodes[0] =
		tfs_inode_table_lock(&self->inode_table, TFS_FS_ROOT_IDX, path.len == 0 ? access : TfsRwLockAccessShared);

	TfsFsFindResult result = tfs_fs_lock_all_from(self, path, locked_inodes[0], locked_inodes + 1, access);
	if (!result.success) { tfs_inode_table_unlock_inode(&self->inode_table, TFS_FS_ROOT_IDX); }

	return result;
}

TfsFs tfs_fs_new(void) {
	const size_t inode_table_size = 128;

	// Create the inode table
	TfsFs fs = {.inode_table = tfs_inode_table_new(inode_table_size)};

	// Create the root node and unlock it
	TfsInodeIdx idx = tfs_inode_table_add(&fs.inode_table, TfsInodeTypeDir);
	assert(idx.idx == TFS_FS_ROOT_IDX.idx);
	tfs_inode_table_unlock_inode(&fs.inode_table, idx);

	return fs;
}

void tfs_fs_destroy(TfsFs* self) {
	// Destroy the inode table
	tfs_inode_table_destroy(&self->inode_table);
}

TfsFsCreateResult tfs_fs_create(TfsFs* const self, TfsPath path, TfsInodeType type) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name = tfs_path_pop_last(path, &parent_path);

	// All locked inodes, for each component
	const size_t locked_inodes_len = tfs_path_components_len(parent_path) + 1;
	TfsLockedInode locked_inodes[locked_inodes_len];

	// Find the parent inode
	TfsFsFindResult find_parent_result = tfs_fs_lock_all(self, parent_path, locked_inodes, TfsRwLockAccessUnique);
	if (!find_parent_result.success) {
		return (TfsFsCreateResult){
			.success = false,
			.data.err.kind = TfsFsCreateErrorInexistentParentDir,
			.data.err.data.inexistent_parent_dir.err = find_parent_result.data.err,
		};
	}

	// If the parent isn't a directory, return Err
	TfsLockedInode parent = find_parent_result.data.inode;
	if (parent.type != TfsInodeTypeDir) {
		// Unlock all inodes locked so far
		for (size_t n = 0; n < locked_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
		}

		return (TfsFsCreateResult){
			.success = false,
			.data.err.kind = TfsFsCreateErrorParentNotDir,
		};
	}

	// Create the new inode
	TfsInodeIdx idx = tfs_inode_table_add(&self->inode_table, type);

	// And try to add it to the inode table
	TfsInodeDirAddEntryResult add_entry_result =
		tfs_inode_dir_add_entry(&parent.data->dir, idx, entry_name.chars, entry_name.len);
	if (!add_entry_result.success) {
		// Unlock all inodes locked so far
		for (size_t n = 0; n < locked_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
		}

		// Remove the inode we created
		tfs_inode_table_remove_inode(&self->inode_table, idx);

		return (TfsFsCreateResult){
			.success = false,
			.data.err.kind = TfsFsCreateErrorAddEntry,
			.data.err.data.add_entry.err = add_entry_result.data.err,
		};
	}

	// Unlock all inodes locked so far (except the child)
	for (size_t n = 0; n < locked_inodes_len; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
	}
	return (TfsFsCreateResult){.success = true, .data.idx = idx};
}

TfsFsRemoveResult tfs_fs_remove(TfsFs* self, TfsPath path) {
	// Split the path into a filename and it's parent directories.
	TfsPath parent_path;
	TfsPath entry_name = tfs_path_pop_last(path, &parent_path);

	// All locked inodes, for each component
	const size_t locked_inodes_len = tfs_path_components_len(parent_path) + 1;
	TfsLockedInode locked_inodes[locked_inodes_len];

	// Find the parent inode
	TfsFsFindResult find_parent_result = tfs_fs_lock_all(self, parent_path, locked_inodes, TfsRwLockAccessUnique);
	if (!find_parent_result.success) {
		return (TfsFsRemoveResult){
			.success = false,
			.data.err.kind = TfsFsRemoveErrorInexistentParentDir,
			.data.err.data.inexistent_parent_dir.err = find_parent_result.data.err,
		};
	}

	// If the parent isn't a directory, return Err
	TfsLockedInode parent = find_parent_result.data.inode;
	if (parent.type != TfsInodeTypeDir) {
		// Unlock all inodes locked so far
		for (size_t n = 0; n < locked_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
		}

		return (TfsFsRemoveResult){
			.success = false,
			.data.err.kind = TfsFsRemoveErrorParentNotDir,
		};
	}

	// Try to find the inode index we need to delete, if we can't, return Err.
	TfsInodeDirSearchByNameResult find_child_result =
		tfs_inode_dir_search_by_name(&parent.data->dir, entry_name.chars, entry_name.len);
	if (!find_child_result.success) {
		// Unlock all inodes locked so far
		for (size_t n = 0; n < locked_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
		}

		return (TfsFsRemoveResult){
			.success = false,
			.data.err.kind = TfsFsRemoveErrorNameNotFound,
		};
	}

	// Lock the inode we're deleting with unique access to ensure no one else is
	// using it.
	// SAFETY: As we have the parent locked, we guarantee `idx` will
	//         stay alive until we can lock it for deletion.
	TfsLockedInode child =
		tfs_inode_table_lock(&self->inode_table, find_child_result.data.success.idx, TfsRwLockAccessUnique);

	// If it's a directory but it's not empty, return Err
	if (child.type == TfsInodeTypeDir && !tfs_inode_dir_is_empty(&child.data->dir)) {
		// Unlock all inodes locked so far
		for (size_t n = 0; n < locked_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
		}
		tfs_inode_table_unlock_inode(&self->inode_table, child.idx);

		return (TfsFsRemoveResult){
			.success = false,
			.data.err.kind = TfsFsRemoveErrorRemoveNonEmptyDir,
		};
	}

	// Else remove it from the directory
	// SAFETY: We got `dir_idx` from `search_by_name`.
	tfs_inode_dir_remove_entry_by_dir_idx(&parent.data->dir, find_child_result.data.success.dir_idx);

	// Remove it from the table and unlock all inodes locked so far.
	tfs_inode_table_remove_inode(&self->inode_table, child.idx);
	for (size_t n = 0; n < locked_inodes_len; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
	}
	return (TfsFsRemoveResult){.success = true};
}

TfsFsFindResult tfs_fs_find(TfsFs* self, TfsPath path, TfsRwLockAccess access) {
	// All locked inodes, for each component
	const size_t locked_inodes_len = tfs_path_components_len(path) + 1;
	TfsLockedInode locked_inodes[locked_inodes_len];

	// Find the inode
	TfsFsFindResult result = tfs_fs_lock_all(self, path, locked_inodes, access);
	if (!result.success) { return result; }

	// Unlock all inodes except the last and return
	for (size_t n = 0; n < locked_inodes_len - 1; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_inodes[n].idx);
	}

	return result;
}

TfsFsMoveResult tfs_fs_move(TfsFs* self, TfsPath orig_path, TfsPath dest_path, TfsRwLockAccess access) {
	// Get the common ancestor of both paths
	TfsPath orig_path_rest;
	TfsPath dest_path_rest;
	TfsPath common_ancestor_path = tfs_path_common_ancestor(orig_path, dest_path, &orig_path_rest, &dest_path_rest);

	// Split the rests
	TfsPath orig_path_parent;
	TfsPath orig_path_filename = tfs_path_pop_last(orig_path_rest, &orig_path_parent);

	TfsPath dest_path_parent;
	TfsPath dest_path_filename = tfs_path_pop_last(dest_path_rest, &dest_path_parent);

	// If the origin path is the destination path's parent, or backwards, return Err
	if (tfs_path_eq(orig_path, dest_path_parent)) {
		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorOriginDestinationParent,
		};
	}
	if (tfs_path_eq(orig_path_parent, dest_path)) {
		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorDestinationOriginParent,
		};
	}

	// Lock up until the common path normally
	// Note: If the common ancestor is one of the parents, we lock it with unique.
	const size_t locked_common_inodes_len = tfs_path_components_len(common_ancestor_path) + 1;
	TfsLockedInode locked_common_inodes[locked_common_inodes_len];
	TfsFsFindResult common_result = tfs_fs_lock_all(self,
		common_ancestor_path,
		locked_common_inodes,
		orig_path_parent.len == 0 || dest_path_parent.len == 0 ? TfsRwLockAccessUnique : TfsRwLockAccessShared //
	);
	if (!common_result.success) {
		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorInexistentCommonAncestor,
		};
	}

	// If all rests are empty, return success, as we're moving a file to itself
	// Note: We do it here because we need to lock the inode to return it
	if (dest_path_rest.len == 0 && orig_path_rest.len == 0) {
		// Unlock everything except the last
		for (size_t n = 0; n < locked_common_inodes_len - 1; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}

		// Note: `len >= 1`, so this is always valid.
		return (TfsFsMoveResult){
			.success = true,
			.data.inode = locked_common_inodes[locked_common_inodes_len - 1],
		};
	}

	// Else if it isn't a directory, return Err
	TfsLockedInode common_ancestor = common_result.data.inode;
	if (common_ancestor.type != TfsInodeTypeDir) {
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}
		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorCommonAncestorNotDir,
		};
	}

	// If both parents are the same, simply rename the file
	// Note: If both parents are the same, they'll both be empty and the common ancestor is the parent
	if (orig_path_parent.len == 0 && dest_path_parent.len == 0) {
		// Find the child
		TfsInodeDirSearchByNameResult search_result =
			tfs_inode_dir_search_by_name(&common_ancestor.data->dir, orig_path_filename.chars, orig_path_filename.len);
		if (!search_result.success) {
			for (size_t n = 0; n < locked_common_inodes_len; n++) {
				tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
			}
			return (TfsFsMoveResult){
				.success = false,
				.data.err.kind = TfsFsMoveErrorOriginNotFound,
			};
		}

		// Lock the child
		TfsLockedInode child =
			tfs_inode_table_lock(&self->inode_table, search_result.data.success.idx, TfsRwLockAccessUnique);

		// Rename it
		TfsInodeDirRenameResult rename_result = tfs_inode_dir_rename(
			&common_ancestor.data->dir, child.idx, dest_path_filename.chars, dest_path_filename.len);
		if (!rename_result.success) {
			for (size_t n = 0; n < locked_common_inodes_len; n++) {
				tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
			}
			tfs_inode_table_unlock_inode(&self->inode_table, child.idx);
			return (TfsFsMoveResult){
				.success = false,
				.data.err.kind = TfsFsMoveErrorRenameEntry,
				.data.err.data.rename_entry.err = rename_result.data.err,
			};
		}

		// Unlock all inodes (except child inode)
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}

		return (TfsFsMoveResult){
			.success = true,
			.data.inode = child,
		};
	}

	// All locked inodes, for each parent component
	// Note: Not including the common parent again.
	const size_t locked_orig_inodes_len = tfs_path_components_len(orig_path_parent);
	TfsLockedInode locked_orig_inodes[locked_orig_inodes_len];
	const size_t locked_dest_inodes_len = tfs_path_components_len(dest_path_parent);
	TfsLockedInode locked_dest_inodes[locked_dest_inodes_len];

	// Then lock each path in a deterministic order.
	TfsLockedInode orig_parent;
	TfsLockedInode dest_parent;
	if (tfs_str_cmp(orig_path_parent.chars, orig_path_parent.len, dest_path_parent.chars, dest_path_parent.len) < 0) {
		if (orig_path_parent.len == 0) { orig_parent = common_ancestor; }
		else {
			TfsFsFindResult orig_parent_result = tfs_fs_lock_all_from(
				self, orig_path_parent, common_ancestor, locked_orig_inodes, TfsRwLockAccessUnique);
			if (!orig_parent_result.success) {
				for (size_t n = 0; n < locked_common_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
				}
				return (TfsFsMoveResult){
					.success = false,
					.data.err.kind = TfsFsMoveErrorInexistentOriginParentDir,
				};
			}
			orig_parent = orig_parent_result.data.inode;
		}

		if (dest_path_parent.len == 0) { dest_parent = common_ancestor; }
		else {
			TfsFsFindResult dest_parent_result = tfs_fs_lock_all_from(
				self, dest_path_parent, common_ancestor, locked_dest_inodes, TfsRwLockAccessUnique);
			if (!dest_parent_result.success) {
				for (size_t n = 0; n < locked_common_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
				}
				for (size_t n = 0; n < locked_orig_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
				}
				return (TfsFsMoveResult){
					.success = false,
					.data.err.kind = TfsFsMoveErrorInexistentDestinationParentDir,
				};
			}
			dest_parent = dest_parent_result.data.inode;
		}
	}
	else {
		if (dest_path_parent.len == 0) { dest_parent = common_ancestor; }
		else {
			TfsFsFindResult dest_parent_result = tfs_fs_lock_all_from(
				self, dest_path_parent, common_ancestor, locked_dest_inodes, TfsRwLockAccessUnique);
			if (!dest_parent_result.success) {
				for (size_t n = 0; n < locked_common_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
				}
				return (TfsFsMoveResult){
					.success = false,
					.data.err.kind = TfsFsMoveErrorInexistentDestinationParentDir,
				};
			}
			dest_parent = dest_parent_result.data.inode;
		}

		if (orig_path_parent.len == 0) { orig_parent = common_ancestor; }
		else {
			TfsFsFindResult orig_parent_result = tfs_fs_lock_all_from(
				self, orig_path_parent, common_ancestor, locked_orig_inodes, TfsRwLockAccessUnique);
			if (!orig_parent_result.success) {
				for (size_t n = 0; n < locked_common_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
				}
				for (size_t n = 0; n < locked_dest_inodes_len; n++) {
					tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
				}
				return (TfsFsMoveResult){
					.success = false,
					.data.err.kind = TfsFsMoveErrorInexistentOriginParentDir,
				};
			}
			orig_parent = orig_parent_result.data.inode;
		}
	}

	// If one of the parents isn't a directory, return Err
	if (orig_parent.type != TfsInodeTypeDir) {
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_orig_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_dest_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
		}

		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorOriginParentNotDir,
		};
	}
	if (dest_parent.type != TfsInodeTypeDir) {
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_orig_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_dest_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
		}

		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorDestinationParentNotDir,
		};
	}

	// Find the origin file
	TfsInodeDirSearchByNameResult search_result =
		tfs_inode_dir_search_by_name(&orig_parent.data->dir, orig_path_filename.chars, orig_path_filename.len);
	if (!search_result.success) {
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_orig_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_dest_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
		}

		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorOriginNotFound,
		};
	}

	// Lock the origin file
	TfsLockedInode orig = tfs_inode_table_lock(&self->inode_table, search_result.data.success.idx, access);

	// Add the file to the new directory
	TfsInodeDirAddEntryResult add_entry_result =
		tfs_inode_dir_add_entry(&dest_parent.data->dir, orig.idx, dest_path_filename.chars, dest_path_filename.len);
	if (!add_entry_result.success) {
		for (size_t n = 0; n < locked_common_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_orig_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
		}
		for (size_t n = 0; n < locked_dest_inodes_len; n++) {
			tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
		}

		return (TfsFsMoveResult){
			.success = false,
			.data.err.kind = TfsFsMoveErrorAddEntry,
			.data.err.data.add_entry.err = add_entry_result.data.err,
		};
	}

	// Remove the source entry
	tfs_inode_dir_remove_entry_by_dir_idx(&orig_parent.data->dir, search_result.data.success.dir_idx);

	// Release all locks (except the source's lock)
	for (size_t n = 0; n < locked_common_inodes_len; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_common_inodes[n].idx);
	}
	for (size_t n = 0; n < locked_orig_inodes_len; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_orig_inodes[n].idx);
	}
	for (size_t n = 0; n < locked_dest_inodes_len; n++) {
		tfs_inode_table_unlock_inode(&self->inode_table, locked_dest_inodes[n].idx);
	}

	// Return the index
	// Note: We return it locked.
	return (TfsFsMoveResult){
		.success = true,
		.data.inode = orig,
	};
}

TfsFsPrintResult tfs_fs_print(TfsFs* self, const char* file_name) {
	// Open the file for writing
	FILE* out = fopen(file_name, "w");

	if (out == NULL) {
		return (TfsFsPrintResult){
			.success = false,
			.data.err.kind = TfsFsPrintErrorCreate,
		};
	}

	// Lock the root for unique access
	tfs_inode_table_lock(&self->inode_table, TFS_FS_ROOT_IDX, TfsRwLockAccessUnique);

	// Print the root inode and all it's children
	// Note: We start off with '' as the root, instead of '/'.
	tfs_inode_table_print_tree(&self->inode_table, TFS_FS_ROOT_IDX, out, "");

	// Lock the root for unique access
	tfs_inode_table_unlock_inode(&self->inode_table, TFS_FS_ROOT_IDX);

	// Close the file and unlock
	assert(fclose(out) == 0);

	return (TfsFsPrintResult){.success = true};
}

void tfs_fs_unlock_inode(TfsFs* self, TfsInodeIdx idx) {
	// Simply delegate to the inode table
	// Note: it will check if `idx` is valid, so we don't have to.
	tfs_inode_table_unlock_inode(&self->inode_table, idx);
}
