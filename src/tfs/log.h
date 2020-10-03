/// @file
/// @brief Debugging logging utilities
/// @details
/// This file provides debugging logging utilities.
/// These are useful for quickly catching an error without
/// having to debug the whole program, or when debugging the
/// programming results in different results from running it
/// normally.
///
/// Based on https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c.

// Imports
#include <stdio.h> // fprintf, stderr

// If `DEBUG` isn't defined, set it as 0
// Note: We disable warnings for defining `DEBUG`, as
//       we aren't responsible for it, we just define it
//       if it's not defined by someone else yet.
/// @cond Doxygen_Suppress
#ifndef DEBUG
	#define DEBUG 0
#endif
/// @endcond

/// @brief Prints a debug log message to stderr with formatting
/// @details
/// The file and line will be printed alongside the rest,
/// prepended using '%s:%zu: '.
/// A newline will also be appended to the format string.
#define TFS_DEBUG_LOG(fmt, ...)                                                       \
	do {                                                                              \
		if (DEBUG) {                                                                  \
			fprintf(stderr, "%s:%03u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
		}                                                                             \
	} while (0)
