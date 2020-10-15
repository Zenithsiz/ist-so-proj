#include "path.h"

// Includes
#include <ctype.h>	  // isspace
#include <stdlib.h>	  // malloc, free
#include <string.h>	  // strlen, strncpy
#include <tfs/util.h> // tfs_str_eq

TfsPath tfs_path_from_cstr(const char* cstr) {
	// Get the length, exclusing the null pointer
	size_t len = strlen(cstr);

	TfsPath path = {.chars = cstr, .len = len};
	return path;
}

TfsPathOwned tfs_path_to_owned(TfsPath path) {
	// Copy over the string
	char* chars = malloc(path.len * sizeof(char));
	strncpy(chars, path.chars, path.len);

	return (TfsPathOwned){
		.chars = chars,
		.len   = path.len,
	};
}

TfsPath tfs_path_from_owned(TfsPathOwned path) {
	return (TfsPath){
		.chars = path.chars,
		.len   = path.len,
	};
}

void tfs_path_owned_destroy(TfsPathOwned* self) {
	// Free the string and set it to `NULL`.
	free(self->chars);
	self->chars = NULL;
}

bool tfs_path_eq(TfsPath lhs, TfsPath rhs) {
	tfs_path_trim(&lhs);
	tfs_path_trim(&rhs);

	return tfs_str_eq(lhs.chars, lhs.len, rhs.chars, rhs.len);
}

void tfs_path_split_last(TfsPath self, TfsPath* parent, TfsPath* child) {
	// Trim `self`
	tfs_path_trim(&self);

	// Find the position of the last separator
	size_t sep_pos = (size_t)-1;
	for (size_t n = 0; n < self.len; ++n) {
		if (self.chars[n] == '/') {
			sep_pos = n;
		}
	}

	// Set both paths
	if (parent != NULL) {
		parent->chars = self.chars;
		// Note: If we didn't find any separator, parent should have length 0
		parent->len = (sep_pos == (size_t)-1 ? 0 : sep_pos);
	}

	if (child != NULL) {
		// If we didn't find any separator, set the child to the whole path
		if (sep_pos == (size_t)-1) {
			*child = self;
		}
		// Else just skip the separator and set the child as the rest
		else {
			child->chars = self.chars + sep_pos + 1;
			child->len	 = self.len - sep_pos - 1;
		}
	}
}

void tfs_path_split_first(TfsPath self, TfsPath* parent, TfsPath* child) {
	// Trim `self`
	tfs_path_trim(&self);

	// Find the first separator
	size_t sep_pos = (size_t)-1;
	for (size_t n = 0; n < self.len; ++n) {
		if (self.chars[n] == '/') {
			sep_pos = n;
			break;
		}
	}

	// Set both paths
	if (parent != NULL) {
		// If we didn't find any separator, set the parent as the whole path
		if (sep_pos == (size_t)-1) {
			*parent = self;
		}

		// Else set it until the separator, not including it
		else {
			parent->chars = self.chars;
			parent->len	  = sep_pos;
		}
	}

	if (child != NULL) {
		// If we didn't find any separator, set the child as empty
		if (sep_pos == (size_t)-1) {
			child->chars = self.chars + self.len;
			child->len	 = 0;
		}

		// Else skip the separator
		else {
			child->chars = self.chars + sep_pos + 1;
			child->len	 = self.len - sep_pos - 1;
		}
	}
}

void tfs_path_trim(TfsPath* self) {
	while (self->len > 0 && isspace(self->chars[0])) {
		self->chars++;
		self->len--;
	}

	// Remove any trailing slashes and whitespace
	while (self->len > 0 && (self->chars[self->len - 1] == '/' || isspace(self->chars[self->len - 1]))) {
		self->len--;
	}
}
