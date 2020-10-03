#include "dir.h"

// Includes
#include <string.h>	  // strcmp
#include <tfs/util.h> // TFS_MIN, TFS_MAX

void tfs_inode_dir_add_entry_result_print(const TfsInodeDirAddEntryResult* self, FILE* out) {
	switch (self->kind) {
		case TfsInodeDirAddEntryResultErrorEmptyName: {
			fprintf(out, "Cannot add an entry with an empty name\n");
			break;
		}

		case TfsInodeDirAddEntryResultErrorDuplicateName:
			fprintf(out, "A path with the same name, with inode index %zu, already exists\n", self->data.duplicate_name.idx);
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

void tfs_inode_dir_drop(TfsInodeDir* self) {
	// Free our entries
	// Note: This is fine even if it's `NULL`.
	free(self->entries);
}

bool tfs_inode_dir_is_empty(const TfsInodeDir* self) {
	// Check every entry, if we find a non-empty one, we're not empty
	for (size_t n = 0; n < self->capacity; n++) {
		if (self->entries[n].inode_idx != TFS_INODE_IDX_NONE) {
			return false;
		}
	}

	// If we got here, they were all empty
	return true;
}

TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* self, const char* name, size_t name_len) {
	// If the name matches on any entry and it's not empty, return it
	for (size_t n = 0; n < self->capacity; n++) {
		// If we're empty, skip
		if (self->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			continue;
		}

		// If the names are different, continue
		if (strncmp(self->entries[n].name, name, TFS_MIN(name_len, TFS_DIR_MAX_FILE_NAME_LEN)) != 0) {
			continue;
		}

		// Else return the index
		return self->entries[n].inode_idx;
	}

	// If we got here, none of them had the same name
	return TFS_INODE_IDX_NONE;
}

bool tfs_inode_dir_remove_entry(TfsInodeDir* self, TfsInodeIdx idx) {
	// Check each entry until we find the one with index `idx`
	for (size_t n = 0; n < self->capacity; n++) {
		if (self->entries[n].inode_idx == idx) {
			// Then wipe it's index and name.
			self->entries[n].inode_idx = TFS_INODE_IDX_NONE;
			self->entries[n].name[0]   = '\0';
			return true;
		}
	}

	// If we got here, there was no entry with the index, so return Err
	return false;
}

TfsInodeDirAddEntryResult tfs_inode_dir_add_entry(TfsInodeDir* self, TfsInodeIdx idx, const char* name, size_t name_len) {
	// If the name is empty, return Err
	if (name_len == 0) {
		return (TfsInodeDirAddEntryResult){.kind = TfsInodeDirAddEntryResultErrorEmptyName};
	}

	// Search for both an empty entry and any duplicates
	size_t empty_idx = (size_t)-1;
	for (size_t n = 0; n < self->capacity; n++) {
		// If we're empty, if we haven't found an empty index yet, set it
		if (self->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			if (empty_idx == (size_t)-1) {
				empty_idx = n;
			}
		}

		// Else check if we're adding a duplicate
		else {
			size_t entry_len = strlen(self->entries[n].name);
			if (entry_len == name_len && strncmp(self->entries[n].name, name, TFS_MIN(entry_len, name_len)) == 0) {
				return (TfsInodeDirAddEntryResult){
					.kind = TfsInodeDirAddEntryResultErrorDuplicateName,
					.data = {.duplicate_name = {.idx = self->entries[n].inode_idx}}};
			}
		}
	}

	// If we didn't find any empty entries, reallocate and retry
	if (empty_idx == (size_t)-1) {
		// Double the current capacity so we don't allocate often
		// Note: We allocate at least 4 because `2 + 0 == 0`.
		size_t new_capacity = TFS_MAX((size_t)4, 2 * self->capacity);

		// Try to allocate
		// Note: It's fine even if `dir->entries` is `NULL`
		TfsDirEntry* new_entries = realloc(self->entries, new_capacity * sizeof(TfsDirEntry));
		if (new_entries == NULL) {
			fprintf(stderr, "Unable to expand directory capacity to %zu\n", new_capacity);
			exit(EXIT_FAILURE);
		}

		// Set all new entries as empty
		// Note: We skip the first, as we'll initialize it after this.
		for (size_t n = self->capacity + 1; n < new_capacity; n++) {
			new_entries[n].name[0]	 = '\0';
			new_entries[n].inode_idx = TFS_INODE_IDX_NONE;
		}

		// Set the index as the first new index and continue
		empty_idx = self->capacity;

		// Move everything into `dir`
		self->entries  = new_entries;
		self->capacity = new_capacity;
	}

	// Else set it's node and copy the name
	self->entries[empty_idx].inode_idx = idx;

	size_t min_len = TFS_MIN(name_len, TFS_DIR_MAX_FILE_NAME_LEN);
	strncpy(self->entries[empty_idx].name, name, min_len);
	self->entries[empty_idx].name[min_len] = '\0';

	return (TfsInodeDirAddEntryResult){.kind = TfsInodeDirAddEntryResultSuccess};
}
