/// @file
/// @brief `TfsInodeDir` tests

// Imports
#include <stdio.h>			 // printf
#include <stdlib.h>			 // size_t, EXIT_SUCCESS
#include <string.h>			 // strncmp
#include <tfs/inode/dir.h>	 // TfsPath
#include <tfs/log.h>		 // TFS_DEBUG_LOG
#include <tfs/test/assert.h> // TFS_ASSERT_OR_RETURN
#include <tfs/test/test.h>	 // TfsTest, TfsTestFn, TfsTestResult

int main(void) {
	// All tests
	TfsTest* tests = (TfsTest[]){
		(TfsTest){.fn = NULL}};

	if (tfs_test_all(tests, "dir") == TfsTestResultSuccess) {
		return EXIT_SUCCESS;
	}
	else {
		return EXIT_FAILURE;
	}
}
