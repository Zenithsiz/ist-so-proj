/// @file
/// @brief File system paths
/// @details
/// This file contains the @ref TfsPath type, which
/// is used to refer to all paths within the file system.

// Imports
#include <stdbool.h> // bool
#include <stdlib.h>	 // size_t

/// @brief A file system path
/// @details
/// This path stores it's own length, instead of
/// being null terminated, so splitting operations,
/// such as getting the file name, or the base name
/// do not need to modify the initial path.
typedef struct TfsPath {
	/// @brief All characters
	const char* chars;

	/// @brief Number of characters
	size_t len;
} TfsPath;

/// @brief Creates a new path from a null-terminated string
TfsPath tfs_path_from_cstr(const char* cstr);

/// @brief Checks if two paths are equal.
/// @details
/// This ignores any number of trailing slashes,
/// as well as any whitespace on either side.
bool tfs_path_eq(TfsPath lhs, TfsPath rhs);

/// @brief Splits this path at it's final component
/// @details
/// Trailing slashes are ignored, and the final slash
/// is not included in either the parent or child.
/// If a path only contains 1 component, the parent's
/// path is empty.
///
/// The following table shows the return values
/// for some common paths:
/// | `path`    | `parent` | `child` |
/// |-----------|----------|---------|
/// | "a/b/c"   | "a/b"    | "c"     |
/// | "a/b/c/"  | ^        | ^       |
/// | "/a/b/c"  | "/a/b"   | ^       |
/// | "/a/b/c/" | ^        | ^       |
/// | "/c"      | ""       | ^       |
/// | "/c/"     | ^        | ^       |
/// | "/"       | ^        | ""      |
/// | ""        | ^        | ^       |
void tfs_path_split_last(TfsPath path, TfsPath* parent, TfsPath* child);

/// @brief Splits this path at it's final component
/// @details
/// First slash is not included in either the parent or child.
/// If a path only contains 1 component, the child's
/// path is empty.
///
/// The following table shows the return values
/// for some common paths:
/// | `path`    | `parent` | `child`  |
/// |-----------|----------|----------|
/// | "a/b/c"   | "a"      | "b/c"    |
/// | "a/b/c/"  | ^        | "b/c/"   |
/// | "a"       | ^        | ""       |
/// | "a/"      | ^        | ^        |
/// | "/a/b/c"  | ""       | "a/b/c"  |
/// | "/a/b/c/" | ^        | "a/b/c/" |
/// | "/c"      | ^        | "c"      |
/// | "/c/"     | ^        | "c/"     |
/// | "/"       | ^        | ""       |
/// | ""        | ^        | ^        |
void tfs_path_split_first(TfsPath path, TfsPath* parent, TfsPath* child);
