/// @file
/// @brief File system paths
/// @details
/// This file defines the #TfsPath type, which
/// is used to refer to all paths within the file system.

#ifndef TFS_PATH_H
#define TFS_PATH_H

// Imports
#include <stdbool.h> // bool
#include <stddef.h>	 // size_t

/// @brief A file system path
/// @details
/// Each path is made up of several components,
/// separated by any number of forward slashes ('/')
/// and whitespace characters.
///
/// This path stores it's own length, instead of
/// being null terminated, so slicing operations,
/// don't need to modify the original string.
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

/// @brief Creates a borrow to this path.
TfsPath tfs_path_owned_borrow(TfsPathOwned self);

/// @brief Destroys a #TfsPathOwned
/// @param self
void tfs_path_owned_destroy(TfsPathOwned* self);

/// @brief Creates a new path from a null-terminated string
TfsPath tfs_path_from_cstr(const char* cstr);

/// @brief Copies this path into an owned buffer.
TfsPathOwned tfs_path_to_owned(TfsPath self);

/// @brief Checks if two paths are equal.
/// @details
/// Two paths are considered equal if they both
/// contain the same components.
bool tfs_path_eq(TfsPath lhs, TfsPath rhs);

/// @brief Trims this path.
/// @details
/// This will remove any leading and trailing whitespace
/// and slashes.
/// The returned path has the properly that it compares
/// equal to the original path using #tfs_path_eq
/// @note
/// As the file system has no notion of relative paths,
/// the paths '/a' and 'a' are equal.
TfsPath tfs_path_trim(TfsPath self);

/// @brief Pops the first component of this path.
/// @param self
/// @param[out] rest
/// Contains the leftover components of @p self
/// May be null.
/// @return The first component.
/// @details
/// Both the returned value and @p rest will be trimmed
TfsPath tfs_path_pop_first(TfsPath self, TfsPath* rest);

/// @brief Pops the last component of this path.
/// @param self
/// @param[out] rest
/// Contains the leftover components of @p self
/// May be null
/// @return The last component.
/// @details
/// Both the returned value and @p rest will be trimmed
TfsPath tfs_path_pop_last(TfsPath self, TfsPath* rest);

/// @brief Returns the number of components in this path
size_t tfs_path_components_len(TfsPath self);

/// @brief Retrieves the common ancestor of two paths
/// @details
/// The common ancestor of two paths is defined as the
/// first common parent of @p lhs and @p rhs , that is,
/// if the paths given are 'a/b/c' and 'a/d', the common
/// ancestor would be 'a', as it is the first parent that
/// both paths share.
/// @param lhs
/// @param rhs
/// @param[out] lhs_rest Contains the leftover components of @p lhs . May be null.
/// @param[out] rhs_rest Contains the leftover components of @p rhs . May be null.
TfsPath tfs_path_common_ancestor(TfsPath lhs, TfsPath rhs, TfsPath* lhs_rest, TfsPath* rhs_rest);

#endif
