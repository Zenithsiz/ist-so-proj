/// @file
/// @brief Testing utilities for tfs

#ifndef TFS_TEST_TEST_H
#define TFS_TEST_TEST_H

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
/// If any test returns failure, this function returns a failure
/// Prints to stdout the status of each function and it's name.
/// @param tests A null terminated (via the `fn` member) list of functions.
/// @param module The module name to append to the test names.
TfsTestResult tfs_test_all(const TfsTest* tests, const char* module);

#endif
