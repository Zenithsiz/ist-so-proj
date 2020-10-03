#include "path.h"

// Includes
#include <ctype.h>	// isspace
#include <string.h> // strlen

TfsPath tfs_path_from_cstr(const char* cstr) {
	// Get the length, exclusing the null pointer
	size_t len = strlen(cstr);

	TfsPath path = {.chars = cstr, .len = len};
	return path;
}

bool tfs_path_eq(TfsPath lhs, TfsPath rhs) {
	// Remove any leading whitespace
	while (lhs.len > 0 && isspace(lhs.chars[0])) {
		lhs.chars++;
		lhs.len--;
	}
	while (rhs.len > 0 && isspace(rhs.chars[0])) {
		rhs.chars++;
		rhs.len--;
	}

	// Remove any trailing slashes and whitespace
	while (lhs.len > 0 && (lhs.chars[lhs.len - 1] == '/' || isspace(lhs.chars[lhs.len - 1]))) {
		lhs.len--;
	}
	while (rhs.len > 0 && (rhs.chars[rhs.len - 1] == '/' || isspace(rhs.chars[rhs.len - 1]))) {
		rhs.len--;
	}

	return lhs.len == rhs.len && strncmp(lhs.chars, rhs.chars, lhs.len) == 0;
}

void tfs_path_split_last(TfsPath path, TfsPath* parent, TfsPath* child) {
	// Remove any trailing slashes from `path`.
	if (path.len > 0 && path.chars[path.len - 1] == '/') {
		path.len--;
	}

	// Find the position of the last separator
	size_t sep_pos = (size_t)-1;
	for (size_t n = 0; n < path.len; ++n) {
		if (path.chars[n] == '/') {
			sep_pos = n;
		}
	}

	// Set both paths
	if (parent != NULL) {
		parent->chars = path.chars;
		// Note: If we didn't find any separator, parent should have length 0
		parent->len = (sep_pos == (size_t)-1 ? 0 : sep_pos);
	}

	if (child != NULL) {
		// If we didn't find any separator, set the child to the whole path
		if (sep_pos == (size_t)-1) {
			*child = path;
		}
		// Else just skip the separator and set the child as the rest
		else {
			child->chars = path.chars + sep_pos + 1;
			child->len	 = path.len - sep_pos - 1;
		}
	}
}

void tfs_path_split_first(TfsPath path, TfsPath* parent, TfsPath* child) {
	// Find the first separator
	size_t sep_pos = (size_t)-1;
	for (size_t n = 0; n < path.len; ++n) {
		if (path.chars[n] == '/') {
			sep_pos = n;
			break;
		}
	}

	// Set both paths
	if (parent != NULL) {
		// If we didn't find any separator, set the parent as the whole path
		if (sep_pos == (size_t)-1) {
			*parent = path;
		}

		// Else set it until the separator, not including it
		else {
			parent->chars = path.chars;
			parent->len	  = sep_pos;
		}
	}

	if (child != NULL) {
		// If we didn't find any separator, set the child as empty
		if (sep_pos == (size_t)-1) {
			child->chars = path.chars + path.len;
			child->len	 = 0;
		}

		// Else skip the separator
		else {
			child->chars = path.chars + sep_pos + 1;
			child->len	 = path.len - sep_pos - 1;
		}
	}
}
