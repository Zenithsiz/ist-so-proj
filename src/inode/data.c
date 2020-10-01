#include "data.h"

// Includes
#include <string.h> // strcmp

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
			return TfsInodeDataDirErrorSuccess;
		}
	}
	return TfsInodeDataDirErrorNoNameMatch;
}
