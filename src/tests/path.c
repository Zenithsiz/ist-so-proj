/// @file
/// @brief `TfsPath` tests

// Imports
#include <stdlib.h>		// size_t, EXIT_SUCCESS
#include <string.h>		// strncmp
#include <tests/util.h> // TFS_ASSERT_OR_RETURN
#include <tfs/path.h>	// TfsPath

static int from_c_str(void) {
	const char* path_cstr = "/my/path/";
	size_t path_cstr_len  = strlen(path_cstr);
	TfsPath path		  = tfs_path_from_cstr(path_cstr);

	TFS_ASSERT_EQ_OR_RETURN(strncmp(path.chars, path_cstr, path_cstr_len), 0);
	TFS_ASSERT_EQ_OR_RETURN(path.len, path_cstr_len);

	return EXIT_SUCCESS;
}

int main(void) {
	int (*tests[])(void) = {from_c_str, NULL};

	for (size_t n = 0;; n++) {
		if (tests[n] == NULL) {
			break;
		}

		if (tests[n]() == EXIT_FAILURE) {
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
