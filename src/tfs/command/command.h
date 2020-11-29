/// @file
/// @brief Executable commands in the file system.
/// @details
/// This file defines the type #TfsCommand that fully describes
/// all commands executable by the tfs server.

#ifndef TFS_COMMAND_COMMAND_H
#define TFS_COMMAND_COMMAND_H

// Includes
#include <stdio.h>			// FILE
#include <tfs/inode/type.h> // TfsInodeType
#include <tfs/path.h>		// TfsPathOwned

/// @brief All executable commands
typedef struct TfsCommand {
	/// @brief Type of command
	enum {
		/// @brief Creates a file.
		/// @details
		/// This command adds a file with path `path`
		/// and inode type `type`.
		TfsCommandCreate,

		/// @brief Searches for a file.
		/// @details
		/// This command searches the filesystem for
		/// a file with path `path`.
		TfsCommandSearch,

		/// @brief Removes a file.
		/// @details
		/// This command deletes the file at the path given
		TfsCommandRemove,

		/// @brief Removes a file.
		/// @details
		/// This command moves a file to another path.
		TfsCommandMove,
	} kind;

	/// @brief Data for all commands
	union {
		/// @brief Data for `Create` command
		struct {
			/// @brief The path to create the file in
			TfsPathOwned path;

			/// @brief Inode type to create
			TfsInodeType type;
		} create;

		/// @brief Data for `Search` command
		struct {
			/// @brief The path of the file to search
			TfsPathOwned path;
		} search;

		/// @brief Data for `Remove` command
		struct {
			/// @brief The path of the file to remove
			TfsPathOwned path;
		} remove;

		/// @brief Data for `Move` command
		struct {
			/// @brief The source path of the file to move
			TfsPathOwned source;

			/// @brief The destination path of the file to move
			TfsPathOwned dest;
		} move;
	} data;
} TfsCommand;

/// @brief Error type for #tfs_command_parse
typedef struct TfsCommandParseError {
	/// @brief Error kind
	enum {
		/// @brief Unable to read line
		TfsCommandParseErrorReadLine,

		/// @brief Missing command argument
		TfsCommandParseErrorNoCommand,

		/// @brief Invalid command
		TfsCommandParseErrorInvalidCommand,

		/// @brief Missing arguments for `Create` command.
		TfsCommandParseErrorMissingCreateArgs,

		/// @brief Invalid inode type for `Create` command.
		TfsCommandParseErrorInvalidType,

		/// @brief Missing arguments for `Search` command.
		TfsCommandParseErrorMissingSearchArgs,

		/// @brief Missing arguments for `Remove` command.
		TfsCommandParseErrorMissingRemoveArgs,

		/// @brief Missing arguments for `Move` command.
		TfsCommandParseErrorMissingMoveArgs,
	} kind;

	/// @brief Error data
	union {
		/// @brief Data for `InvalidCommand`.
		struct {
			/// @brief Character received
			char command;
		} invalid_command;

		/// @brief Data for `InvalidType`.
		struct {
			/// @brief Character received, '\0' is length isn't 1
			char type;

			/// @brief Length of the string received
			size_t len;
		} invalid_type;
	} data;
} TfsCommandParseError;

/// @brief Result type for #tfs_command_parse
typedef struct TfsCommandParseResult {
	/// @brief If successful
	bool success;

	/// @brief Result data
	union {
		/// @brief Success command
		TfsCommand command;

		/// @brief Underlying error
		TfsCommandParseError err;
	} data;
} TfsCommandParseResult;

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out);

/// @brief Parses a command from a space-separated argument list
/// @param in The file to read from
TfsCommandParseResult tfs_command_parse(FILE* in);

/// @brief Serializes this command to a string
void tfs_command_to_string(const TfsCommand* command, char* buffer, size_t buffer_len);

/// @brief Destroys a command
void tfs_command_destroy(TfsCommand* command);

#endif
