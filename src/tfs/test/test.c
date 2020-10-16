#include "test.h"

// Includes
#include <stdlib.h> // size_t

TfsTestResult tfs_test_all(const TfsTest* tests, FILE* out) {
	// Current status
	TfsTestResult status = TfsTestResultSuccess;

	// Run all tests
	for (size_t n = 0; tests[n].fn != NULL; n++) {
		fprintf(out, "%s:\t", tests[n].name);
		if (tests[n].fn() == TfsTestResultSuccess) {
			fprintf(out, "Passed\n");
		}
		else {
			status = TfsTestResultFailure;
			fprintf(out, "Failed\n");
		}
	}

	return status;
}
