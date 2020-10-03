#include "test.h"

// Includes
#include <stdio.h>	// printf
#include <stdlib.h> // size_t

TfsTestResult tfs_test_all(const TfsTest* tests, const char* module) {
	// Current status
	TfsTestResult status = TfsTestResultSuccess;

	// Run all tests
	printf("Tests '%s':\n", module);
	for (size_t n = 0;; n++) {
		if (tests[n].fn == NULL) {
			break;
		}

		printf("\t%s/%s: ..", module, tests[n].name);
		if (tests[n].fn() == TfsTestResultSuccess) {
			printf("Passed\n");
		}
		else {
			status = TfsTestResultFailure;
			printf("Failed\n");
		}
	}

	return status;
}
