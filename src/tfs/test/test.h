/// @file
/// @brief Testing utilities
/// @details
/// This file defines various testing utilities used by
/// the tests in `src/tests`.

#ifndef TFS_TEST_TEST_H
#define TFS_TEST_TEST_H

// Imports
#include <stdio.h> // FILE

/// @brief A test result
typedef enum TfsTestResult {
	/// @brief Failure
	TfsTestResultFailure = 0,

	/// @brief Success
	TfsTestResultSuccess = 1,
} TfsTestResult;

/// @brief A test function
typedef TfsTestResult (*TfsTestFn)(void);

/// @brief A test function along with it's name
typedef struct TfsTest {
	/// @brief The function
	TfsTestFn fn;

	/// @brief The name
	const char* name;
} TfsTest;

/// @brief Runs a series of tests
/// @details
/// If any test returns failure, this function will continue executing
/// other tests, but will return @ref TfsTestResultFailure.
/// Prints to @p out the result of each test along with it's name.
/// @param tests A null terminated (via the `fn` member) list of functions.
/// @param out File to print test results to.
TfsTestResult tfs_test_all(const TfsTest* tests, FILE* out);

#endif
