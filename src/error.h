/// @file
/// Errors

#ifndef TFS_ERRORS_H
#define TFS_ERRORS_H

/// @brief Generic error type.
typedef enum TfsError {
	/// Success, no error
	TfsErrorSuccess = 0,

	/// Client already has an open session with a server
	TfsErrorOpenSession = -1,

	/// No open session exists
	TfsErrorNoOpenSession = -2,

	/// Connection error
	TfsErrorConnectionError = -3,

	/// A file with the given name already exists
	TfsErrorFileAlreadyExists = -4,

	/// No file found with the given name
	TfsErrorFileNotFound = -5,

	/// Insufficient permissions
	TfsErrorPermissionDenied = -6,

	/// Max number of open files reached
	TfsErrorMaxedOpenFiles = -7,

	/// File is not open
	TfsErrorFileNotOpen = -8,

	/// File is open
	TfsErrorFileIsOpen = -9,

	/// File open mode does not allow this operation
	TfsErrorInvalidMode = -10,

	/// Other error
	TfsErrorOther = -11,
} TfsError;

#endif
