/// @file
/// @brief Executable commands in the file system.
/// @details
/// This file contains the type @ref TfsCommand that fully describes
/// all commands executable by the tfs server.

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

/// @brief Result type for `tfs_command_parse`
typedef struct TfsCommandParseResult {
	/// @brief Result kind
	enum {
		/// @brief No error
		TfsCommandParseResultSuccess,

		/// @brief Missing command argument
		TfsCommandParseResultErrorNoCommand,

		/// @brief Missing path argument
		TfsCommandParseResultErrorNoPath,

		/// @brief Missing type argument
		TfsCommandParseResultErrorNoType,

		/// @brief Invalid command
		TfsCommandParseResultErrorInvalidCommand,

		/// @brief Invalid type
		TfsCommandParseResultErrorInvalidType,
	} kind;

	/// @brief Result data
	union {
		/// @brief Data for `Success` variant.
		struct {
			/// @brief The parsed command
			TfsCommand command;
		} success;
	} data;
} TfsCommandParseResult;

/// @brief Parses a command from a newline-terminated space-separated argument list.
TfsCommandParseResult tfs_command_parse(FILE* in);

/// @brief Destroys a command
void tfs_command_destroy(TfsCommand command);
