/// @file
/// @brief Utilities
/// @details
/// This file contains miscellaneous utilities
/// such as `min`

/// @brief Returns the minimum value between `lhs` and `rhs`
/// @details
/// Adapted from https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define TFS_MIN(lhs, rhs)          \
	__extension__({                \
		typeof(lhs) _lhs = (lhs);  \
		typeof(rhs) _rhs = (rhs);  \
		_lhs > _rhs ? _rhs : _lhs; \
	})

/// @brief Returns the maximum value between `lhs` and `rhs`
/// @details
/// Adapted from https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define TFS_MAX(lhs, rhs)          \
	__extension__({                \
		typeof(lhs) _lhs = (lhs);  \
		typeof(rhs) _rhs = (rhs);  \
		_lhs > _rhs ? _lhs : _rhs; \
	})
