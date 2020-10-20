#include "path.h"

// Includes
#include <ctype.h>	  // isspace
#include <stdio.h>	  // fprintf, stderr
#include <stdlib.h>	  // malloc, free
#include <string.h>	  // strlen, strncpy
#include <tfs/util.h> // tfs_str_eq

/// @brief Checks if @p ch is either a forward slash, '/',
///        or space, according to #isspace
static inline bool is_slash_or_space(int ch) {
	return ch == '/' || isspace(ch);
}

TfsPath tfs_path_owned_borrow(TfsPathOwned self) {
	return (TfsPath){
		.chars = self.chars,
		.len   = self.len,
	};
}

void tfs_path_owned_destroy(TfsPathOwned* self) {
	// Free the string and set it to `NULL`.
	// Note: `free` accepts null pointers.
	free(self->chars);
	self->chars = NULL;
}

TfsPath tfs_path_from_cstr(const char* cstr) {
	return (TfsPath){
		.chars = cstr,
		// Note: `strlen` doesn't include the null terminator
		.len = strlen(cstr),
	};
}

TfsPathOwned tfs_path_to_owned(TfsPath self) {
	// If the string is empty, return an empty path
	if (self.len == 0) {
		return (TfsPathOwned){
			.chars = NULL,
			.len   = 0,
		};
	}

	// Else copy the string over.
	// Note: Using memcpy as we don't consider the null terminator.
	char* chars = malloc(self.len * sizeof(char));
	if (chars == NULL) {
		fprintf(stderr, "Unable to allocate owned path with buffer size %zu", self.len);
	}
	memcpy(chars, self.chars, self.len);

	return (TfsPathOwned){
		.chars = chars,
		.len   = self.len,
	};
}

bool tfs_path_eq(TfsPath lhs, TfsPath rhs) {
	do {
		// Pop the first component on both
		TfsPath lhs_first = tfs_path_pop_first(lhs, &lhs);
		TfsPath rhs_first = tfs_path_pop_first(rhs, &rhs);

		// If any component is empty, return if both are empty.
		if (lhs_first.len == 0 || rhs_first.len == 0) {
			return lhs_first.len == 0 && rhs_first.len == 0;
		}

		// If they're different, return false
		// Note: They've been trimmed by `pop_first`.
		if (!tfs_str_eq(lhs_first.chars, lhs_first.len, rhs_first.chars, rhs_first.len)) {
			return false;
		}
	} while (1);
}

TfsPath tfs_path_trim(TfsPath self) {
	// Remove any leading slashes and whitespace
	while (self.len > 0 && is_slash_or_space(self.chars[0])) {
		self.chars++;
		self.len--;
	}

	// Remove any trailing slashes and whitespace
	while (self.len > 0 && is_slash_or_space(self.chars[self.len - 1])) {
		self.len--;
	}
	return self;
}

TfsPath tfs_path_pop_first(TfsPath self, TfsPath* rest) {
	// Trim self
	self = tfs_path_trim(self);

	// Find the first '/' to split at.
	size_t first_sep_pos = (size_t)-1;
	for (size_t n = 0; n < self.len; n++) {
		if (self.chars[n] == '/') {
			first_sep_pos = n;
			break;
		}
	}

	// If we didn't find it, there is only 1 component, return it
	// and set the rest as empty.
	if (first_sep_pos == (size_t)-1) {
		if (rest != NULL) { *rest = (TfsPath){.chars = self.chars + self.len, .len = 0}; }
		return self;
	}

	// Else split the two at the first slash and trim
	// Note: Trimming is required on `self` as the
	//       path may end with whitespace.
	if (rest != NULL) {
		// Note: Fine to include the slash, as it'll get
		//       removed by `trim`.
		*rest = tfs_path_trim((TfsPath){
			.chars = self.chars + first_sep_pos,
			// Note: This is always positive due to `first_sep_pos < len`.
			.len = self.len - first_sep_pos,
		});
	}
	return tfs_path_trim((TfsPath){
		.chars = self.chars,
		.len   = first_sep_pos,
	});
}

TfsPath tfs_path_pop_last(TfsPath self, TfsPath* rest) {
	// Trim self
	self = tfs_path_trim(self);

	// Find the last '/' to split at.
	size_t last_sep_pos = (size_t)-1;
	for (size_t n = self.len - 1; n != (size_t)-1; n--) {
		if (self.chars[n] == '/') {
			last_sep_pos = n;
			break;
		}
	}

	// If we didn't find it, there is only 1 component, return it
	// and set the rest as empty.
	if (last_sep_pos == (size_t)-1) {
		if (rest != NULL) { *rest = (TfsPath){.chars = self.chars + self.len, .len = 0}; }
		return self;
	}

	// Else split the two at the last slash and trim
	// Note: Trimming is required on `self` as the
	//       path may end with whitespace.
	if (rest != NULL) {
		*rest = tfs_path_trim((TfsPath){
			.chars = self.chars,
			.len   = last_sep_pos,
		});
	}
	// Note: Fine to include the slash, as it'll get
	//       removed by `trim`.
	return tfs_path_trim((TfsPath){
		.chars = self.chars + last_sep_pos,
		.len   = self.len - last_sep_pos,
	});
}

TfsPath tfs_path_common_ancestor(const TfsPath lhs, const TfsPath rhs, TfsPath* const lhs_rest, TfsPath* const rhs_rest) {
	TfsPath cur_lhs = lhs;
	TfsPath cur_rhs = rhs;

	do {
		// Save the old paths
		TfsPath old_lhs = cur_lhs;
		TfsPath old_rhs = cur_rhs;

		// Pop the first component on both
		TfsPath cur_lhs_first = tfs_path_pop_first(cur_lhs, &cur_lhs);
		TfsPath cur_rhs_first = tfs_path_pop_first(cur_rhs, &cur_rhs);

		// If either both paths are now empty or they're different, return.
		if ((cur_lhs_first.len == 0 && cur_rhs_first.len == 0) || !tfs_str_eq(cur_lhs_first.chars, cur_lhs_first.len, cur_rhs_first.chars, cur_rhs_first.len)) {
			if (lhs_rest != NULL) { *lhs_rest = old_lhs; }
			if (rhs_rest != NULL) { *rhs_rest = old_rhs; }
			return tfs_path_trim((TfsPath){
				.chars = lhs.chars,
				.len   = (size_t)(old_lhs.chars - lhs.chars),
			});
		}
	} while (1);
}
