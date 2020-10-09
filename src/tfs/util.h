/// @file
/// @brief Utilities
/// @details
/// This file contains miscellaneous utilities
/// such as `min`

#ifndef TFS_UTIL_H
#define TFS_UTIL_H

// Includes
#include <stdbool.h> // bool
#include <stdlib.h>	 // size_t

/// @brief Returns the minimum value between `lhs` and `rhs`
inline static size_t tfs_min_size_t(size_t lhs, size_t rhs) {
	return lhs > rhs ? rhs : lhs;
}

/// @brief Returns the maximum value between `lhs` and `rhs`
inline static size_t tfs_max_size_t(size_t lhs, size_t rhs) {
	return lhs > rhs ? lhs : rhs;
}

#endif
