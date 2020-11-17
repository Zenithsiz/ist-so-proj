/// @file
/// @brief Testing assertions
/// @details
/// This file provides assertions that return
/// @ref TfsTestResultFailure from the current
/// function if the given condition is false.

#ifndef TFS_TEST_ASSERT_H
#define TFS_TEST_ASSERT_H

// Imports
#include <stdio.h>		   // fprintf, stderr
#include <tfs/test/test.h> // TfsTestResult

/// @brief Asserts a condition is true, else returns @ref TfsTestResultFailure
/// @details
/// It also prints a message to stderr with the condition stringified
#define TFS_ASSERT_OR_RETURN(cond)                               \
	do {                                                         \
		if (!(cond)) {                                           \
			fprintf(stderr, "Condition failed:\n\t" #cond "\n"); \
			return TfsTestResultFailure;                         \
		}                                                        \
	} while (0)

/// @brief Asserts 2 values are equal, else returns @ref TfsTestResultFailure
/// @details
/// This is a simple wrapper over `TFS_ASSERT_OR_RETURN(lhs == rhs)`
#define TFS_ASSERT_EQ_OR_RETURN(lhs, rhs) TFS_ASSERT_OR_RETURN(lhs == rhs)

#endif
