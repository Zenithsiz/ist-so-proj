/// @file
/// @brief Testing utilities
/// @details
/// This file provides utilities for
/// test programs to use.

// Imports
#include <stdio.h>	// fprintf, stderr
#include <stdlib.h> // EXIT_FAILURE

/// @brief Asserts a condition is true, else returns `EXIT_FAILURE`
/// @details
/// It also prints a message to stderr with the condition stringified
#define TFS_ASSERT_OR_RETURN(cond)                               \
	do {                                                         \
		if (!(cond)) {                                           \
			fprintf(stderr, "Condition failed:\n\t" #cond "\n"); \
			return EXIT_FAILURE;                                 \
		}                                                        \
	} while (0)

/// @brief Asserts 2 values are equal, else returns `EXIT_FAILURE`
/// @details
/// This is a simple wrapper over `TFS_ASSERT_OR_RETURN(lhs == rhs)`
#define TFS_ASSERT_EQ_OR_RETURN(lhs, rhs) \
	do {                                  \
		TFS_ASSERT_OR_RETURN(lhs == rhs); \
	} while (0)
