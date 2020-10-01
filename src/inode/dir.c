#include "dir.h"

// Includes
#include <inode/type.h> // TfsInodeType
#include <string.h>		// strcmp

int tfs_inode_dir_is_empty(TfsInodeDir* dir) {
	// For each entry, if we find a non-empty entry, return false
	for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx != TfsInodeIdxNone) {
			return 0;
		}
	}

	// If we got here, they were all empty
	return 1;
}

TfsInodeDataDirError tfs_inode_dir_search_by_name(TfsInodeDir* dir, const char* name, size_t name_len, TfsInodeIdx* idx) {
	// For each entry, if it's not empty, and the name matches, return it
	for (size_t n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx != TfsInodeIdxNone && strncmp(dir->entries[n].name, name, name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len) == 0) {
			if (idx != NULL) {
				*idx = dir->entries[n].inode_idx;
			}
			return TfsInodeDirErrorSuccess;
		}
	}
	return TfsInodeDataDirErrorNoNameMatch;
}

TfsInodeDataDirError tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx) {
	// Find the entry with index `idx`
	for (int n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx == idx) {
			// Then set it to none and remove it's name
			dir->entries[n].inode_idx = TfsInodeIdxNone;
			dir->entries[n].name[0]	  = '\0';
			return TfsInodeDirErrorSuccess;
		}
	}

	// If we got here, there was no entry with the sub index, so return Err
	return TfsInodeDataDirErrorIdxNotFound;
}
TfsInodeDataDirError tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len) {
	// If the name is empty, return Err
	if (name_len == 0) {
		return TfsInodeDataDirErrorNameEmpty;
	}

	// Else search for an empty directory entry
	for (int n = 0; n < TFS_DIR_MAX_ENTRIES; n++) {
		if (dir->entries[n].inode_idx == TfsInodeIdxNone) {
			// Set it's index and copy the name
			dir->entries[n].inode_idx = idx;
			strncpy(dir->entries[n].name, name, name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len);
			return EXIT_SUCCESS;
		}
	}

	// If we got here, there was no empty entry
	return TfsInodeDataDirErrorFull;
}
