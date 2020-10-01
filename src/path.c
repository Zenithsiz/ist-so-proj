#include "path.h"

// Includes
#include <string.h> // strlen

TfsPath tfs_path_from_cstr(const char* cstr) {
	// Get the length, exclusing the null pointer
	size_t len = strlen(cstr);

	TfsPath path = {.chars = cstr, .len = len};
	return path;
}

void tfs_path_split_last(TfsPath path, TfsPath* parent, TfsPath* child) {
	// Remove any trailing slashes from `path`.
	if (path.len > 0 && path.chars[path.len - 1] == '/') {
		path.len--;
	}

	// Register the total number of separators, and the position of the last
	size_t total_sep = 0, last_sep_pos = 0;
	for (size_t n = 0; n < path.len; ++n) {
		if (path.chars[n] == '/') {
			last_sep_pos = n;
			total_sep++;
		}
	}

	// Set both paths
	if (parent != NULL) {
		parent->chars = path.chars;
		parent->len	  = last_sep_pos;
	}

	if (child != NULL) {
		child->chars = path.chars + last_sep_pos + 1;
		child->len	 = path.len - last_sep_pos - 1;
	}
}

void tfs_path_split_first(TfsPath path, TfsPath* parent, TfsPath* child) {
	// Remove any leading slashes from `path`.
	if (path.len > 0 && path.chars[0] == '/') {
		path.chars++;
		path.len--;
	}

	// Find the first separator
	size_t sep_pos = path.len;
	for (size_t n = 0; n < path.len; ++n) {
		if (path.chars[n] == '/') {
			sep_pos = n;
			break;
		}
	}

	// Set both paths
	if (parent != NULL) {
		parent->chars = path.chars;
		parent->len	  = sep_pos;
	}

	if (child != NULL) {
		// Offset to skip the separator if it existed
		// Note: This is required so we don't end up with `-1` length, when `sep_pos == path.len`.
		size_t sep_offset = (sep_pos == path.len ? 0 : 1);

		child->chars = path.chars + sep_pos + sep_offset;
		child->len	 = path.len - sep_pos - sep_offset;
	}
}
