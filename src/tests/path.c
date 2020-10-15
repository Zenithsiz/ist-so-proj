/// @file
/// @brief `TfsPath` tests

// Imports
#include <stdio.h>			 // printf
#include <stdlib.h>			 // size_t, EXIT_SUCCESS, EXIT_FAILURE
#include <tfs/path.h>		 // TfsPath
#include <tfs/test/assert.h> // TFS_ASSERT_OR_RETURN
#include <tfs/test/test.h>	 // TfsTest, TfsTestFn, TfsTestResult
#include <tfs/util.h>		 // tfs_str_eq

// Helper typedefs to define arrays of arrays of strings
typedef const char* String;
typedef String* StringArr;

static TfsTestResult from_c_str(void) {
	const char* path_cstr = "/my/path/";
	size_t path_cstr_len  = strlen(path_cstr);
	TfsPath path		  = tfs_path_from_cstr(path_cstr);

	// Make sure the path and the original cstring are equal
	TFS_ASSERT_OR_RETURN(tfs_str_eq(path.chars, path.len, path_cstr, path_cstr_len));

	return TfsTestResultSuccess;
}

static TfsTestResult eq(void) {
	/// All paths that are supposed to be equal
	StringArr* eq_paths = (StringArr[]){
		(String[]){"a", "a"},
		(String[]){"a", "a/"},
		(String[]){"a", "a//"},
		(String[]){"a/b", "a/b"},
		(String[]){"a/b", "  a/b  "},
		(String[]){"a/b", "a/b/"},
		(String[]){"a/b", "  a/b/  "},
		(String[]){"a/b", "a/b//"},
		(String[]){"/", ""},
		(String[]){"/", "  "},
		(String[]){"//", ""},
		(String[]){"//", "  "},
		(String[]){"", ""},
		(String[]){"", "  "},
		NULL};

	for (size_t n = 0;; n++) {
		if (eq_paths[n] == NULL) {
			break;
		}

		TfsPath lhs = tfs_path_from_cstr(eq_paths[n][0]);
		TfsPath rhs = tfs_path_from_cstr(eq_paths[n][1]);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(lhs, rhs));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult diff(void) {
	/// All paths that are suppoed to be different
	StringArr* diff_paths = (StringArr[]){
		(String[]){"a", "b"},
		(String[]){"a/twowords", "a/two words/"},
		NULL};

	for (size_t n = 0;; n++) {
		if (diff_paths[n] == NULL) {
			break;
		}

		TfsPath lhs = tfs_path_from_cstr(diff_paths[n][0]);
		TfsPath rhs = tfs_path_from_cstr(diff_paths[n][1]);

		TFS_ASSERT_OR_RETURN(!tfs_path_eq(lhs, rhs));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult split_last(void) {
	// All paths to test
	// Order: Path, parent, child
	StringArr* paths = (StringArr[]){
		(String[]){"a/b/c", "a/b", "c"},
		(String[]){"a/b/c/", "a/b", "c"},
		(String[]){"/a/b/c", "/a/b", "c"},
		(String[]){"/a/b/c/", "/a/b", "c"},
		(String[]){"/c", "", "c"},
		(String[]){"/c/", "", "c"},
		(String[]){"/", "", ""},
		(String[]){"", "", ""},
		NULL};

	for (size_t n = 0;; n++) {
		if (paths[n] == NULL) {
			break;
		}

		TfsPath path			= tfs_path_from_cstr(paths[n][0]);
		TfsPath expected_parent = tfs_path_from_cstr(paths[n][1]);
		TfsPath expected_child	= tfs_path_from_cstr(paths[n][2]);

		TfsPath parent;
		TfsPath child;
		tfs_path_split_last(path, &parent, &child);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(parent, expected_parent));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(child, expected_child));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult split_first(void) {
	// All paths to test
	// Order: Path, parent, child
	StringArr* paths = (StringArr[]){
		(String[]){"a/b/c", "a", "b/c"},
		(String[]){"a/b/c/", "a", "b/c/"},
		(String[]){"a", "a", ""},
		(String[]){"a/", "a", ""},
		(String[]){"/a/b/c", "", "a/b/c"},
		(String[]){"/a/b/c/", "", "a/b/c/"},
		(String[]){"/c", "", "c"},
		(String[]){"/c/", "", "c/"},
		(String[]){"/", "", ""},
		(String[]){"", "", ""},
		NULL};

	for (size_t n = 0;; n++) {
		if (paths[n] == NULL) {
			break;
		}

		TfsPath path			= tfs_path_from_cstr(paths[n][0]);
		TfsPath expected_parent = tfs_path_from_cstr(paths[n][1]);
		TfsPath expected_child	= tfs_path_from_cstr(paths[n][2]);

		TfsPath parent;
		TfsPath child;
		tfs_path_split_first(path, &parent, &child);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(parent, expected_parent));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(child, expected_child));
	}

	return TfsTestResultSuccess;
}

int main(void) {
	// All tests
	TfsTest* tests = (TfsTest[]){
		(TfsTest){.fn = from_c_str, .name = "from_c_str"},
		(TfsTest){.fn = eq, .name = "eq"},
		(TfsTest){.fn = diff, .name = "diff"},
		(TfsTest){.fn = split_last, .name = "split_last"},
		(TfsTest){.fn = split_first, .name = "split_first"},
		(TfsTest){.fn = NULL}};

	if (tfs_test_all(tests, "path") == TfsTestResultSuccess) {
		return EXIT_SUCCESS;
	}
	else {
		return EXIT_FAILURE;
	}
}
