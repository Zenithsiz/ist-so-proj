#include "dir.h"

// Includes
#include <assert.h>	  // assert
#include <stdlib.h>	  // malloc, realloc, free
#include <string.h>	  // memcpy
#include <tfs/util.h> // tfs_str_eq

/// @brief Helper function to duplicate a string, like `strndup`
/// @param s The string to copy
/// @param len Length of @p s
/// @return A non-null heap allocated copy of `s`.
static char* tfs_str_dup(const char* s, size_t len) {
	char* new = malloc(len * sizeof(char));
	// Note: We use memcpy, as `s` isn't null terminated
	memcpy(new, s, len * sizeof(char));

	return new;
}

void tfs_inode_dir_add_entry_error_print(const TfsInodeDirAddEntryError* self, FILE* out) {
	switch (self->kind) {
		case TfsInodeDirAddEntryErrorEmptyName: {
			fprintf(out, "Entry name must not be empty\n");
			break;
		}

		case TfsInodeDirAddEntryErrorDuplicateName: {
			fprintf(out, "An entry with the same name (Inode %zu) already exists\n", self->data.duplicate_name.idx);
			break;
		}

		default: {
			break;
		}
	}
}
void tfs_inode_dir_rename_error_print(const TfsInodeDirRenameError* self, FILE* out) {
	switch (self->kind) {
		case TfsInodeDirRenameErrorNotFound: {
			fprintf(out, "Inode with index not found\n");
			break;
		}
		case TfsInodeDirRenameErrorEmptyName: {
			fprintf(out, "Entry name must not be empty\n");
			break;
		}
		case TfsInodeDirRenameErrorDuplicateName: {
			fprintf(out, "An entry with the same name (Inode %zu) already exists\n", self->data.duplicate_name.idx);
			break;
		}
		default: {
			break;
		}
	}
}

TfsInodeDirEntry tfs_inode_dir_entry_new(TfsInodeIdx idx, const char* name, size_t name_len) {
	// Copy the name if it isn't null or empty.
	char* entry_name = tfs_str_dup(name, name_len);

	return (TfsInodeDirEntry){
		.inode_idx = idx,
		.name = entry_name,
		.name_len = name_len,
	};
}

void tfs_inode_dir_entry_destroy(TfsInodeDirEntry* self) {
	// Free the name
	free(self->name);

	// Then set all it's parameters to be empty.
	self->inode_idx = TFS_INODE_IDX_NONE;
	self->name = NULL;
	self->name_len = 0;
}

TfsInodeDir tfs_inode_dir_new(void) {
	return (TfsInodeDir){
		// Note: `NULL` can be safely passed to both `free` and `realloc`.
		.entries = NULL,
		.capacity = 0,
	};
}

void tfs_inode_dir_destroy(TfsInodeDir* self) {
	// Destroy all entries
	for (size_t n = 0; n < self->capacity; ++n) {
		if (self->entries[n].inode_idx != TFS_INODE_IDX_NONE) { tfs_inode_dir_entry_destroy(&self->entries[n]); }
	}

	// Free our entries and set them to NULL
	// Note: This is fine even if it's `NULL`.
	free(self->entries);
	self->entries = NULL;
}

bool tfs_inode_dir_is_empty(const TfsInodeDir* self) {
	// Check every entry, if we find a non-empty one, we're not empty
	for (size_t n = 0; n < self->capacity; n++) {
		if (self->entries[n].inode_idx != TFS_INODE_IDX_NONE) { return false; }
	}

	// If we got here, they were all empty
	return true;
}

TfsInodeDirSearchByNameResult tfs_inode_dir_search_by_name(const TfsInodeDir* self, const char* name, size_t name_len) {
	// If the name matches on any entry and it's not empty, return it
	for (size_t n = 0; n < self->capacity; n++) {
		// If we're empty, skip
		if (self->entries[n].inode_idx == TFS_INODE_IDX_NONE) { continue; }

		// If the names are different, continue
		TfsInodeDirEntry* entry = &self->entries[n];
		if (!tfs_str_eq(name, name_len, entry->name, entry->name_len)) { continue; }

		// Else set the dir idx and return the index
		return (TfsInodeDirSearchByNameResult){
			.success = true,
			.data.success.idx = self->entries[n].inode_idx,
			.data.success.dir_idx = n,
		};
	}

	// If we got here, none of them had the same name
	return (TfsInodeDirSearchByNameResult){.success = false};
}

void tfs_inode_dir_remove_entry_by_dir_idx(TfsInodeDir* self, size_t dir_idx) {
	// Make sure `dir_idx` is valid.
	assert(dir_idx < self->capacity);

	// Then destroy the entry
	tfs_inode_dir_entry_destroy(&self->entries[dir_idx]);
}

TfsInodeDirRenameResult tfs_inode_dir_rename(
	TfsInodeDir* self, TfsInodeIdx idx, const char* new_name, size_t new_name_len //
) {
	// Search for both an empty entry and any duplicates
	size_t dir_idx = (size_t)-1;
	for (size_t n = 0; n < self->capacity; n++) {
		// If we found it, set it.
		if (self->entries[n].inode_idx == idx) {
			dir_idx = n;

			// If it's name is equal to the new name, return success
			if (tfs_str_eq(self->entries[n].name, self->entries[n].name_len, new_name, new_name_len)) {
				return (TfsInodeDirRenameResult){.success = true};
			}
		}

		// Else check if we're adding a duplicate
		else {
			TfsInodeDirEntry* entry = &self->entries[n];
			if (tfs_str_eq(new_name, new_name_len, entry->name, entry->name_len)) {
				return (TfsInodeDirRenameResult){
					.success = false,
					.data.err.kind = TfsInodeDirRenameErrorDuplicateName,
					.data.err.data.duplicate_name.idx = entry->inode_idx,
					.data.err.data.duplicate_name.dir_idx = n,
				};
			}
		}
	}

	// If we didn't find it, return Err
	if (dir_idx == (size_t)-1) {
		return (TfsInodeDirRenameResult){
			.success = false,
			.data.err.kind = TfsInodeDirRenameErrorNotFound,
		};
	}

	// Else rename it
	char* new_entry_name = tfs_str_dup(new_name, new_name_len);
	free(self->entries[dir_idx].name);
	self->entries[dir_idx].name = new_entry_name;
	self->entries[dir_idx].name_len = new_name_len;

	return (TfsInodeDirRenameResult){.success = true};
}

TfsInodeDirAddEntryResult tfs_inode_dir_add_entry(
	TfsInodeDir* self, TfsInodeIdx idx, const char* name, size_t name_len) {
	// If the name is empty, return Err
	if (name_len == 0) {
		return (TfsInodeDirAddEntryResult){
			.success = false,
			.data.err.kind = TfsInodeDirAddEntryErrorEmptyName,
		};
	}

	// Search for both an empty entry and any duplicates
	size_t empty_idx = (size_t)-1;
	for (size_t n = 0; n < self->capacity; n++) {
		// If we're empty, if we haven't found an empty index yet, set it
		if (self->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			if (empty_idx == (size_t)-1) { empty_idx = n; }
		}

		// Else check if we're adding a duplicate
		else {
			TfsInodeDirEntry* entry = &self->entries[n];
			if (tfs_str_eq(name, name_len, entry->name, entry->name_len)) {
				return (TfsInodeDirAddEntryResult){
					.success = false,
					.data.err.kind = TfsInodeDirAddEntryErrorDuplicateName,
					.data.err.data.duplicate_name.idx = entry->inode_idx,
					.data.err.data.duplicate_name.dir_idx = n,
				};
			}
		}
	}

	// If we didn't find any empty entries, reallocate and retry
	if (empty_idx == (size_t)-1) {
		// Double the current capacity so we don't allocate often
		// Note: We allocate at least 4 because `2 * 0 == 0`.
		size_t new_capacity = tfs_max_size_t(4, 2 * self->capacity);

		// Try to allocate
		// Note: It's fine even if `dir->entries` is `NULL`
		TfsInodeDirEntry* new_entries = realloc(self->entries, new_capacity * sizeof(TfsInodeDirEntry));
		if (new_entries == NULL) {
			fprintf(stderr, "Unable to expand directory capacity to %zu\n", new_capacity);
			exit(EXIT_FAILURE);
		}

		// Set all new entries as empty
		// Note: We skip the first, as we'll initialize it after this.
		for (size_t n = self->capacity + 1; n < new_capacity; n++) {
			new_entries[n] = tfs_inode_dir_entry_new(TFS_INODE_IDX_NONE, NULL, 0);
		}

		// Set the index as the first new index and continue
		empty_idx = self->capacity;

		// Move everything into `dir`
		self->entries = new_entries;
		self->capacity = new_capacity;
	}

	// Else create the entry
	self->entries[empty_idx] = tfs_inode_dir_entry_new(idx, name, name_len);

	return (TfsInodeDirAddEntryResult){
		.success = true,
	};
}
