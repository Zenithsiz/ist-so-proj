/// @file
/// @brief Utilities
/// @details
/// This file contains miscellaneous utilities
/// used by the project.

#ifndef TFS_UTIL_H
#define TFS_UTIL_H

// Includes
#include <stdbool.h> // bool
#include <stdlib.h>	 // size_t
#include <string.h>	 // strncmp

/// @brief Returns the minimum value between `lhs` and `rhs`
inline static size_t tfs_min_size_t(size_t lhs, size_t rhs) {
	return lhs > rhs ? rhs : lhs;
}

/// @brief Returns the maximum value between `lhs` and `rhs`
inline static size_t tfs_max_size_t(size_t lhs, size_t rhs) {
	return lhs > rhs ? lhs : rhs;
}

/// @brief Returns if two possibly non-terminated strings are equal
inline static bool tfs_str_eq(const char* lhs, size_t lhs_len, const char* rhs, size_t rhs_len) {
	return lhs_len == rhs_len && strncmp(lhs, rhs, lhs_len) == 0;
}

#endif
