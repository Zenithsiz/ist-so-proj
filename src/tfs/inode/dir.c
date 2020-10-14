#include "dir.h"

// Includes
#include <stdlib.h>	  // malloc, free
#include <string.h>	  // strncmp, strncpy
#include <tfs/util.h> // tfs_min_size_t

void tfs_inode_dir_add_entry_error_print(const TfsInodeDirAddEntryError* self, FILE* out) {
	switch (self->kind) {
		case TfsInodeDirAddEntryErrorEmptyName: {
			fprintf(out, "Cannot add an entry with an empty name\n");
			break;
		}

		case TfsInodeDirAddEntryErrorDuplicateName: {
			fprintf(out, "A path with the same name, with inode index %zu, already exists\n", self->data.duplicate_name.idx);
			break;
		}

		default: {
			break;
		}
	}
}

TfsInodeDirEntry tfs_inode_dir_entry_new(TfsInodeIdx idx, const char* name, size_t name_len) {
	char* entry_name = NULL;
	if (name != NULL && name_len != 0) {
		entry_name = malloc(name_len * sizeof(char));
		strncpy(entry_name, name, name_len);
	}

	return (TfsInodeDirEntry){
		.inode_idx = idx,
		.name	   = entry_name,
		.name_len  = name_len,
	};
}

void tfs_inode_dir_entry_destroy(TfsInodeDirEntry* self) {
	// Free the name
	// Note: Fine even if it's `NULL`.
	free(self->name);

	// Then set all it's parameters to be empty.
	self->inode_idx = TFS_INODE_IDX_NONE;
	self->name		= NULL;
	self->name_len	= 0;
}

TfsInodeDir tfs_inode_dir_new(void) {
	return (TfsInodeDir){
		// Note: `NULL` can be safely passed to both `free` and `realloc`.
		.entries  = NULL,
		.capacity = 0,
	};
}

void tfs_inode_dir_destroy(TfsInodeDir* self) {
	// Destroy all entries
	for (size_t n = 0; n < self->capacity; ++n) {
		if (self->entries[n].inode_idx != TFS_INODE_IDX_NONE) {
			tfs_inode_dir_entry_destroy(&self->entries[n]);
		}
	}

	// Free our entries and set them to NULL
	// Note: This is fine even if it's `NULL`.
	free(self->entries);
	self->entries = NULL;
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

TfsInodeIdx tfs_inode_dir_search_by_name(const TfsInodeDir* self, const char* name, size_t name_len, size_t* dir_idx) {
	// If the name matches on any entry and it's not empty, return it
	for (size_t n = 0; n < self->capacity; n++) {
		// If we're empty, skip
		if (self->entries[n].inode_idx == TFS_INODE_IDX_NONE) {
			continue;
		}

		// If the names are different, continue
		if (name_len != self->entries[n].name_len || strncmp(self->entries[n].name, name, tfs_min_size_t(name_len, self->entries[n].name_len)) != 0) {
			continue;
		}

		// Else set the dir idx and return the index
		if (dir_idx != NULL) {
			*dir_idx = n;
		}
		return self->entries[n].inode_idx;
	}

	// If we got here, none of them had the same name
	return TFS_INODE_IDX_NONE;
}

bool tfs_inode_dir_remove_entry(TfsInodeDir* self, TfsInodeIdx idx) {
	// Check each entry until we find the one with index `idx`
	for (size_t n = 0; n < self->capacity; n++) {
		if (self->entries[n].inode_idx == idx) {
			tfs_inode_dir_entry_destroy(&self->entries[n]);
			return true;
		}
	}

	// If we got here, there was no entry with the index, so return Err
	return false;
}

bool tfs_inode_dir_remove_entry_by_dir_idx(TfsInodeDir* self, size_t dir_idx) {
	// If `dir_idx` is out of range, return Err
	if (dir_idx >= self->capacity) {
		return false;
	}

	// Else destroy the entry
	tfs_inode_dir_entry_destroy(&self->entries[dir_idx]);

	return true;
}

bool tfs_inode_dir_add_entry(TfsInodeDir* self, TfsInodeIdx idx, const char* name, size_t name_len, TfsInodeDirAddEntryError* err) {
	// If the name is empty, return Err
	if (name_len == 0) {
		if (err != NULL) {
			*err = (TfsInodeDirAddEntryError){.kind = TfsInodeDirAddEntryErrorEmptyName};
		}
		return false;
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
			TfsInodeDirEntry* entry = &self->entries[n];
			if (entry->name_len == name_len && strncmp(entry->name, name, tfs_min_size_t(entry->name_len, name_len)) == 0) {
				if (err != NULL) {
					*err = (TfsInodeDirAddEntryError){
						.kind = TfsInodeDirAddEntryErrorDuplicateName,
						.data = {.duplicate_name = {.idx = entry->inode_idx}}};
				}
				return false;
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
		self->entries  = new_entries;
		self->capacity = new_capacity;
	}

	// Else create the entry
	self->entries[empty_idx] = tfs_inode_dir_entry_new(idx, name, name_len);

	return true;
}
