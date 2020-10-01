/// @brief
/// File system paths

// Imports
#include <stdlib.h> // size_t

/// @brief A file system path
/// @details A non-null terminated string
typedef struct TfsPath {
	/// @brief All characters
	const char* chars;

	/// @brief Number of characters
	size_t len;
} TfsPath;

/// @brief Creates a new path from a null-terminated string
TfsPath tfs_path_from_cstr(const char* cstr);

/// @brief Splits this path at it's last component
/// @details If the path only has 1 component, then the parent
///          path will be empty.
///          Trailing slashes are ignored and discarded.
///          The last separator will not appear in either `parent` or `child`.
void tfs_path_split_last(TfsPath path, TfsPath* parent, TfsPath* child);

/// @brief Splits this path at it's first component
/// @details If the path only has 1 component, then the child
///          path will be empty
///          Leading slashes are ignored and discarded.
///          The first separator will not appear in either `parent` or `child`.
void tfs_path_split_first(TfsPath path, TfsPath* parent, TfsPath* child);
