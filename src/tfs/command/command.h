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
	} data;
} TfsCommand;

/// @brief Error type for `tfs_command_parse`
typedef struct TfsCommandParseError {
	/// @brief Error kind
	enum {
		/// @brief Missing command argument
		TfsCommandParseErrorNoCommand,

		/// @brief Missing path argument
		TfsCommandParseErrorNoPath,

		/// @brief Missing type argument
		TfsCommandParseErrorNoType,

		/// @brief Invalid command
		TfsCommandParseErrorInvalidCommand,

		/// @brief Invalid type
		TfsCommandParseErrorInvalidType,
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
			/// @brief Character received
			char type;
		} invalid_type;
	} data;
} TfsCommandParseError;

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out);

/// @brief Parses a command from a space-separated argument list
/// @param in The file to read from
/// @param[out] command Parsed command
/// @param[out] err Set if an error occurs
/// @return If successfully parsed.
bool tfs_command_parse(FILE* in, TfsCommand* command, TfsCommandParseError* err);

/// @brief Destroys a command
void tfs_command_destroy(TfsCommand* command);

#endif
