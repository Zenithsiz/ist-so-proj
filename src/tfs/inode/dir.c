#include "dir.h"

// Includes
#include <string.h> // strcmp

void tfs_inode_dir_add_entry_result_print(const TfsInodeDirAddEntryResult* result, FILE* out) {
	switch (result->kind) {
		case TfsInodeDirAddEntryResultErrorEmptyName: {
			fprintf(out, "Cannot add an entry with an empty name\n");
			break;
		}

		case TfsInodeDirAddEntryResultErrorDuplicateName:
			fprintf(out, "A path with the same name, with inode index '%u', already exists\n", result->data.duplicate_name.idx);
			break;

		case TfsInodeDirAddEntryResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

TfsInodeDir tfs_inode_dir_new(void) {
	return (TfsInodeDir){
		// Note: `NULL` can be safely passed to both `free` and `realloc`.
		.entries  = NULL,
		.capacity = 0,
	};
}

void tfs_inode_dir_drop(TfsInodeDir* dir) {
	// Free our entries
	// Note: This is fine even if it's `NULL`.
	free(dir->entries);
}

bool tfs_inode_dir_is_empty(const TfsInodeDir* dir) {
	// Check every entry, if we find a non-empty one, we're not empty
	for (size_t n = 0; n < dir->capacity; n++) {
		if (dir->entries[n].inode_idx != TFS_INODE_IDX_NONE) {
			return false;
		}
	}

	// If we got here, they were all empty
	return true;
}

TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* dir, const char* name, size_t name_len) {
	// If the name matches on any entry and it's not empty, return it
	for (size_t n = 0; n < dir->capacity; n++) {
		// If we're empty, skip
		if (dir->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			continue;
		}

		// If the names are different, continue
		size_t min_len = name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len;
		if (strncmp(dir->entries[n].name, name, min_len) != 0) {
			continue;
		}

		// Else return the index
		return dir->entries[n].inode_idx;
	}

	// If we got here, none of them had the same name
	return TFS_INODE_IDX_NONE;
}

bool tfs_inode_dir_remove_entry(TfsInodeDir* dir, TfsInodeIdx idx) {
	// Check each entry until we find the one with index `idx`
	for (size_t n = 0; n < dir->capacity; n++) {
		if (dir->entries[n].inode_idx == idx) {
			// Then wipe it's index and name.
			dir->entries[n].inode_idx = TFS_INODE_IDX_NONE;
			dir->entries[n].name[0]	  = '\0';
			return true;
		}
	}

	// If we got here, there was no entry with the index, so return Err
	return false;
}

TfsInodeDirAddEntryResult tfs_inode_dir_add_entry(TfsInodeDir* dir, TfsInodeIdx idx, const char* name, size_t name_len) {
	// If the name is empty, return Err
	if (name_len == 0) {
		return (TfsInodeDirAddEntryResult){.kind = TfsInodeDirAddEntryResultErrorEmptyName};
	}

	// Search for both an empty entry and any duplicates
	size_t empty_idx = (size_t)-1;
	for (size_t n = 0; n < dir->capacity; n++) {
		// If we're empty, if we haven't found an empty index yet, set it
		if (dir->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			if (empty_idx == (size_t)-1) {
				empty_idx = n;
			}
		}

		// Else check if we're adding a duplicate
		else {
			size_t entry_len = strlen(dir->entries[n].name);
			if (entry_len == name_len && strncmp(dir->entries[n].name, name, entry_len > name_len ? name_len : entry_len) == 0) {
				return (TfsInodeDirAddEntryResult){
					.kind = TfsInodeDirAddEntryResultErrorDuplicateName,
					.data = {.duplicate_name = {.idx = dir->entries[n].inode_idx}}};
			}
		}
	}

	// If we didn't find any empty entries, reallocate and retry
	if (empty_idx == (size_t)-1) {
		// Double the current capacity so we don't allocate often
		// Note: We start by allocating 4 to skip having to
		//       the smaller 1 an 2 allocations, as most directories
		//       will likely have at least 4 entries.
		size_t capacity = dir->capacity == 0 ? 4 : 2 * dir->capacity;

		// Try to allocate
		// Note: It's fine even if `dir->entries` is `NULL`
		TfsDirEntry* entries = realloc(dir->entries, capacity * sizeof(TfsDirEntry));
		if (entries == NULL) {
			fprintf(stderr, "Unable to expand directory capacity to %u", capacity);
			exit(EXIT_FAILURE);
		}

		// Set all new entries as empty
		// Note: We skip the first, as we'll initialize it after this.
		for (size_t n = dir->capacity + 1; n < capacity; n++) {
			entries[n].name[0]	 = '\0';
			entries[n].inode_idx = TFS_INODE_IDX_NONE;
		}

		// Set the index as the first new index and continue
		// Note: This new index is guaranteed to be empty.
		empty_idx = dir->capacity;

		// Move everything into `dir`
		dir->entries  = entries;
		dir->capacity = capacity;
	}

	// Else set it's node and copy the name
	dir->entries[empty_idx].inode_idx = idx;

	size_t min_len = name_len > TFS_DIR_MAX_FILE_NAME_LEN ? TFS_DIR_MAX_FILE_NAME_LEN : name_len;
	strncpy(dir->entries[empty_idx].name, name, min_len);
	dir->entries[empty_idx].name[min_len] = '\0';

	return (TfsInodeDirAddEntryResult){.kind = TfsInodeDirAddEntryResultSuccess};
}
