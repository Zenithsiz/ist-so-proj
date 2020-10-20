#include "test.h"

// Includes
#include <stdlib.h> // size_t

TfsTestResult tfs_test_all(const TfsTest* tests, FILE* out) {
	// Current status
	TfsTestResult status = TfsTestResultSuccess;

	// Run all tests
	for (size_t n = 0; tests[n].fn != NULL; n++) {
		if (tests[n].fn() == TfsTestResultSuccess) {
			fprintf(out, "%s:\tPassed\n", tests[n].name);
		}
		else {
			status = TfsTestResultFailure;
			fprintf(out, "%s:\tFailed\n", tests[n].name);
		}
	}

	return status;
}
