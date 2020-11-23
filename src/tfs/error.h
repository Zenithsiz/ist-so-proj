/// @file
/// @brief File system errors
/// @details
/// Errors used for server communication with clients.

#ifndef TFS_ERROR_H
#define TFS_ERROR_H

/// @brief Generic error type.
typedef enum TfsError {
	/// @brief Success, no error
	TfsErrorSuccess = 0,

	/// @brief Client already has an open session with a server
	TfsErrorOpenSession = -1,

	/// @brief No open session exists
	TfsErrorNoOpenSession = -2,

	/// @brief Connection error
	TfsErrorConnectionError = -3,

	/// @brief A file with the given name already exists
	TfsErrorFileAlreadyExists = -4,

	/// @brief No file found with the given name
	TfsErrorFileNotFound = -5,

	/// @brief Insufficient permissions
	TfsErrorPermissionDenied = -6,

	/// @brief Max number of open files reached
	TfsErrorMaxedOpenFiles = -7,

	/// @brief File is not open
	TfsErrorFileNotOpen = -8,

	/// @brief File is open
	TfsErrorFileIsOpen = -9,

	/// @brief File open mode does not allow this operation
	TfsErrorInvalidMode = -10,

	/// @brief Other error
	TfsErrorOther = -11,
} TfsError;

#endif
