/// @file
/// @brief `TfsPath` tests

// Imports
#include <stdio.h>			 // printf
#include <stdlib.h>			 // size_t, EXIT_SUCCESS, EXIT_FAILURE
#include <tfs/path.h>		 // TfsPath
#include <tfs/test/assert.h> // TFS_ASSERT_OR_RETURN
#include <tfs/test/test.h>	 // TfsTest, TfsTestFn, TfsTestResult
#include <tfs/util.h>		 // tfs_str_eq

static TfsTestResult from_c_str(void) {
	const char* cstrs[] = {
		"/my/path/",
		"",
		NULL,
	};

	for (size_t n = 0; cstrs[n] != NULL; n++) {
		const char* cstr = cstrs[n];
		size_t cstr_len	 = strlen(cstr);
		TfsPath path	 = tfs_path_from_cstr(cstr);

		// Make sure the path and the original cstring are equal
		TFS_ASSERT_OR_RETURN(tfs_str_eq(path.chars, path.len, cstr, cstr_len));
	}
	return TfsTestResultSuccess;
}

static TfsTestResult eq(void) {
	// All paths that are supposed to be equal
	const char** eq_paths[] = {
		(const char*[]){"a", "a"},
		(const char*[]){"a", "a/"},
		(const char*[]){"a", "/a/"},
		(const char*[]){"a", "  //  a  //  "},
		(const char*[]){"a/b", "a/b"},
		(const char*[]){"a/b", "/a/b"},
		(const char*[]){"a/b", "/a/b/"},
		(const char*[]){"a/b", " / a / b / "},
		(const char*[]){"/", ""},
		(const char*[]){" ", ""},
		(const char*[]){" / ", ""},
		NULL,
	};

	for (size_t n = 0; eq_paths[n] != NULL; n++) {
		TfsPath lhs = tfs_path_from_cstr(eq_paths[n][0]);
		TfsPath rhs = tfs_path_from_cstr(eq_paths[n][1]);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(lhs, rhs));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult diff(void) {
	// All paths that are suppoed to be different
	const char** diff_paths[] = {
		(const char*[]){"a", "b"},
		(const char*[]){"a/twowords", "a/two words/"},
		NULL,
	};

	for (size_t n = 0; diff_paths[n] != NULL; n++) {
		TfsPath lhs = tfs_path_from_cstr(diff_paths[n][0]);
		TfsPath rhs = tfs_path_from_cstr(diff_paths[n][1]);

		TFS_ASSERT_OR_RETURN(!tfs_path_eq(lhs, rhs));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult pop_last(void) {
	// All paths to test
	// Order: Path, Result, Rest
	const char** paths[] = {
		// clang-format off
		(const char*[]){"a/b/c"  , "c", "a/b" },
		(const char*[]){"a/b/c/" , "c", "a/b" },
		(const char*[]){"/a/b/c" , "c", "/a/b"},
		(const char*[]){"/a/b/c/", "c", "/a/b"},
		(const char*[]){"/c"     , "c", ""    },
		(const char*[]){"/c/"    , "c", ""    },
		(const char*[]){"/"      , "" , ""    },
		(const char*[]){""       , "" , ""    },
		// clang-format on
		NULL,
	};

	for (size_t n = 0; paths[n] != NULL; n++) {
		TfsPath path			= tfs_path_from_cstr(paths[n][0]);
		TfsPath expected_result = tfs_path_from_cstr(paths[n][1]);
		TfsPath expected_rest	= tfs_path_from_cstr(paths[n][2]);

		TfsPath rest;
		TfsPath result = tfs_path_pop_last(path, &rest);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(result, expected_result));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(rest, expected_rest));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult pop_first(void) {
	// All paths to test
	// Order: Path, Result, Rest
	const char** paths[] = {
		// clang-format off
		(const char*[]){"a/b/c"  , "a", "b/c"},
		(const char*[]){"a/b/c/" , "a", "b/c"},
		(const char*[]){"/a/b/c" , "a", "b/c"},
		(const char*[]){"/a/b/c/", "a", "b/c"},
		(const char*[]){"a"      , "a", ""   },
		(const char*[]){"a/"     , "a", ""   },
		(const char*[]){"/a/"    , "a", ""   },
		(const char*[]){"/"      , "" , ""   },
		(const char*[]){""       , "" , ""   },
		// clang-format on
		NULL,
	};

	for (size_t n = 0; paths[n] != NULL; n++) {
		TfsPath path			= tfs_path_from_cstr(paths[n][0]);
		TfsPath expected_result = tfs_path_from_cstr(paths[n][1]);
		TfsPath expected_rest	= tfs_path_from_cstr(paths[n][2]);

		TfsPath rest;
		TfsPath result = tfs_path_pop_first(path, &rest);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(result, expected_result));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(rest, expected_rest));
	}

	return TfsTestResultSuccess;
}

static TfsTestResult common_ancestor(void) {
	// All paths to test
	// Order: Lhs, Rhs, Parent, LhsRest, RhsRest
	const char** paths[] = {
		// clang-format off
		(const char*[]){"a/b/c", "a/d", "a", "b/c"  , "d"},
		(const char*[]){"a"    , "a"  , "a", ""     , "" },
		(const char*[]){"a"    , "b"  , "" , "a"    , "b"},
		(const char*[]){""     , ""   , "" , ""     , "" },
		(const char*[]){"a/b/c", ""   , "" , "a/b/c", "" },
		// clang-format on
		NULL,
	};

	for (size_t n = 0; paths[n] != NULL; n++) {
		TfsPath lhs				  = tfs_path_from_cstr(paths[n][0]);
		TfsPath rhs				  = tfs_path_from_cstr(paths[n][1]);
		TfsPath expected_result	  = tfs_path_from_cstr(paths[n][2]);
		TfsPath expected_lhs_rest = tfs_path_from_cstr(paths[n][3]);
		TfsPath expected_rhs_rest = tfs_path_from_cstr(paths[n][4]);

		TfsPath lhs_rest;
		TfsPath rhs_rest;
		TfsPath result = tfs_path_common_ancestor(lhs, rhs, &lhs_rest, &rhs_rest);

		TFS_ASSERT_OR_RETURN(tfs_path_eq(result, expected_result));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(lhs_rest, expected_lhs_rest));
		TFS_ASSERT_OR_RETURN(tfs_path_eq(rhs_rest, expected_rhs_rest));
	}

	return TfsTestResultSuccess;
}

int main(void) {
	// All tests
	// clang-format off
	TfsTest* tests = (TfsTest[]){
		(TfsTest){.fn = from_c_str     , .name = "path/from_c_str"     },
		(TfsTest){.fn = eq             , .name = "path/eq"             },
		(TfsTest){.fn = diff           , .name = "path/diff"           },
		(TfsTest){.fn = pop_last       , .name = "path/pop_last"       },
		(TfsTest){.fn = pop_first      , .name = "path/pop_first"      },
		(TfsTest){.fn = common_ancestor, .name = "path/common_ancestor"},
		(TfsTest){.fn = NULL},
	};
	// clang-format on

	if (tfs_test_all(tests, stdout) == TfsTestResultSuccess) {
		return EXIT_SUCCESS;
	}
	else {
		return EXIT_FAILURE;
	}
}
