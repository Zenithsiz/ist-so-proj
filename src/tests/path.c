/// @file
/// @brief `TfsPath` tests

// Imports
#include <stdio.h>		// printf
#include <stdlib.h>		// size_t, EXIT_SUCCESS
#include <string.h>		// strncmp
#include <tests/util.h> // TFS_ASSERT_OR_RETURN
#include <tfs/log.h>	// TFS_DEBUG_LOG
#include <tfs/path.h>	// TfsPath

// Helper typedefs to define arrays of arrays of strings
typedef const char* String;
typedef String* StringArr;

static int from_c_str(void) {
	const char* path_cstr = "/my/path/";
	size_t path_cstr_len  = strlen(path_cstr);
	TfsPath path		  = tfs_path_from_cstr(path_cstr);

	TFS_ASSERT_EQ_OR_RETURN(strncmp(path.chars, path_cstr, path_cstr_len), 0);
	TFS_ASSERT_EQ_OR_RETURN(path.len, path_cstr_len);

	return EXIT_SUCCESS;
}

static int eq(void) {
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

		TFS_DEBUG_LOG("'%.*s' == '%.*s'", (int)lhs.len, lhs.chars, (int)rhs.len, rhs.chars);
		TFS_ASSERT_OR_RETURN(tfs_path_eq(lhs, rhs));
	}

	return EXIT_SUCCESS;
}

static int diff(void) {
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

		TFS_DEBUG_LOG("'%.*s' != '%.*s'", (int)lhs.len, lhs.chars, (int)rhs.len, rhs.chars);
		TFS_ASSERT_OR_RETURN(!tfs_path_eq(lhs, rhs));
	}

	return EXIT_SUCCESS;
}

static int split_last(void) {
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

		TFS_DEBUG_LOG("Path        : '%.*s'", (int)path.len, path.chars);
		TFS_DEBUG_LOG("Expected parent: '%.*s'", (int)expected_parent.len, expected_parent.chars);
		TFS_DEBUG_LOG("Parent         : '%.*s'", (int)parent.len, parent.chars);
		TFS_DEBUG_LOG("Expected child: '%.*s'", (int)expected_child.len, expected_child.chars);
		TFS_DEBUG_LOG("Child         : '%.*s'", (int)child.len, child.chars);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(parent, expected_parent));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(child, expected_child));
	}

	return EXIT_SUCCESS;
}

static int split_first(void) {
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

		TFS_DEBUG_LOG("Path: '%.*s'", (int)path.len, path.chars);
		TFS_DEBUG_LOG("Expected parent: '%.*s'", (int)expected_parent.len, expected_parent.chars);
		TFS_DEBUG_LOG("Parent         : '%.*s'", (int)parent.len, parent.chars);
		TFS_DEBUG_LOG("Expected child: '%.*s'", (int)expected_child.len, expected_child.chars);
		TFS_DEBUG_LOG("Child         : '%.*s'", (int)child.len, child.chars);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(parent, expected_parent));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(child, expected_child));
	}

	return EXIT_SUCCESS;
}

typedef int (*TestFn)(void);

/// @brief A test function + name
typedef struct Test {
	/// @brief The function
	TestFn fn;

	/// @brief The name
	const char* name;
} Test;

int main(void) {
	// All tests
	Test* tests = (Test[]){
		(Test){.fn = from_c_str, .name = "from_c_str"},
		(Test){.fn = eq, .name = "eq"},
		(Test){.fn = diff, .name = "diff"},
		(Test){.fn = split_last, .name = "split_last"},
		(Test){.fn = split_first, .name = "split_first"},
		(Test){.fn = NULL}};

	// Run all tests
	for (size_t n = 0;; n++) {
		if (tests[n].fn == NULL) {
			break;
		}

		printf("path/%s: ..", tests[n].name);
		if (tests[n].fn() == EXIT_SUCCESS) {
			printf("Passed\n");
		}
		else {
			printf("Failed\n");
		}
	}

	return EXIT_SUCCESS;
}
