/// @file
/// @brief File system paths
/// @details
/// This file contains the #TfsPath type, which
/// is used to refer to all paths within the file system.

#ifndef TFS_PATH_H
#define TFS_PATH_H

// Imports
#include <stdbool.h> // bool
#include <stddef.h>	 // size_t

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

/// @brief Owned version of #TfsPath
typedef struct TfsPathOwned {
	/// @brief All characters
	char* chars;

	/// @brief Number of characters
	size_t len;
} TfsPathOwned;

/// @brief Creates a new path from a null-terminated string
TfsPath tfs_path_from_cstr(const char* cstr);

/// @brief Creates a new owned path from a path
TfsPathOwned tfs_path_to_owned(TfsPath path);

/// @brief Retrieves a borrowed path from an owned path
/// @param path The path to borrow from.
TfsPath tfs_path_from_owned(TfsPathOwned path);

/// @brief Destroys a #TfsPathOwned
/// @param self
void tfs_path_owned_destroy(TfsPathOwned* self);

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
/// @param self
/// @param parent Pointer to place the parent at. May be null
/// @param child Pointer to place the child at. May be null
void tfs_path_split_last(TfsPath self, TfsPath* parent, TfsPath* child);

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
/// @param self
/// @param parent Pointer to place the parent at. May be null
/// @param child Pointer to place the child at. May be null
void tfs_path_split_first(TfsPath self, TfsPath* parent, TfsPath* child);

/// @brief Trims whitespace and trailing slashes from this path
void tfs_path_trim(TfsPath* self);

#endif
