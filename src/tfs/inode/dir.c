#include "dir.h"

// Includes
#include <string.h>			// strcmp
#include <tfs/inode/type.h> // TfsInodeType

bool tfs_inode_dir_is_empty(TfsInodeDir* dir) {
	// Check every entry, if we find a non-empty one, we're not empty
	for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx != (TfsInodeIdx)TfsInodeIdxNone) {
			return false;
		}
	}

	// If we got here, they were all empty
	return true;
}

TfsInodeDataDirError tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, size_t name_len, TfsInodeIdx* idx) {
	// If the name matches on any entry and it's not empty, return it
	for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		// If we're empty, skip
		if (dir->entries[n].inode_idx == (TfsInodeIdx)TfsInodeIdxNone) {
			continue;
		}

		// If the names are different, continue
		size_t min_len = name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len;
		if (strncmp(dir->entries[n].name, name, min_len) != 0) {
			continue;
		}

		// Else write the index if it isn't null
		if (idx != NULL) {
			*idx = dir->entries[n].inode_idx;
		}
		return TfsInodeDirErrorSuccess;
	}

	// If we got here, none of them had the same name
	return TfsInodeDataDirErrorNoNameMatch;
}

TfsInodeDataDirError tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx) {
	// Check each entry until we find the one with index `idx`
	for (int n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx == idx) {
			// Then wipe it's index and name.
			dir->entries[n].inode_idx = TfsInodeIdxNone;
			dir->entries[n].name[0]	  = '\0';
			return TfsInodeDirErrorSuccess;
		}
	}

	// If we got here, there was no entry with the index, so return Err
	return TfsInodeDataDirErrorIdxNotFound;
}
TfsInodeDataDirError tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len) {
	// If the name is empty, return Err
	if (name_len == 0) {
		return TfsInodeDataDirErrorNameEmpty;
	}

	// Search for both an empty entry and any duplicates
	size_t empty_idx = (size_t)-1;
	for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		// If we're empty, set the empty index
		if (dir->entries[n].inode_idx == (TfsInodeIdx)TfsInodeIdxNone) {
			empty_idx = n;
		}

		// Else check if this is a duplicate
		else {
			if (dir->entries[n].inode_idx == idx) {
				return TfsInodeDataDirErrorDuplicateEntry;
			}
		}
	}

	// If we didn't find any empty entries, return Err
	if (empty_idx == (size_t)-1) {
		return TfsInodeDataDirErrorFull;
	}

	// Else set it's node and copy the name
	dir->entries[empty_idx].inode_idx = idx;

	size_t min_len = name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len;
	strncpy(dir->entries[empty_idx].name, name, min_len);
	dir->entries[empty_idx].name[min_len] = '\0';

	return TfsInodeDirErrorSuccess;
}
