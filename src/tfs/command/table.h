/// @file
/// @brief Command table
/// @details
/// This file provides the @ref TfsCommandTable type,
/// which is a multi-threaded vector of commands.

#ifndef TFS_COMMAND_TABLE_H
#define TFS_COMMAND_TABLE_H

// Includes
#include <stdbool.h>			 // bool
#include <stdlib.h>				 // size_t
#include <tfs/command/command.h> // TfsCommand
#include <tfs/command/lock.h>	 // TfsCommandTableLock

/// @brief Max number of commands
#define TFS_COMMAND_TABLE_MAX 150000

/// @brief The command table
typedef struct TfsCommandTable {
	/// @brief All of the commands
	TfsCommand commands[TFS_COMMAND_TABLE_MAX];

	/// @brief Current number of commands in `commands`
	size_t len;

	/// @brief Lock
	TfsCommandTableLock lock;
} TfsCommandTable;

/// @brief Result for @ref tfs_command_table_push
typedef enum TfsCommandTablePushResult {
	/// @brief Successfully pushed
	TfsCommandTablePushResultSuccess,

	/// @brief Table was full
	TfsCommandTablePushResultErrorFull,
} TfsCommandTablePushResult;

/// @brief Result for @ref tfs_command_table_pop
typedef struct TfsCommandTablePopResult {
	/// @brief If there is some command
	bool is_some;

	/// @brief Result data
	union {
		/// @brief Parsed command
		TfsCommand command;
	} data;
} TfsCommandTablePopResult;

/// @brief Prints a textual representation of an result
/// @param self
/// @param out File descriptor to output to
void tfs_command_table_push_result_print(const TfsCommandTablePushResult* self, FILE* out);

/// @brief Creates a new, empty, command table
/// @details
/// Result is allocated to prevent stack smashing
TfsCommandTable* tfs_command_table_new(TfsCommandTableLockKind lock_kind);

/// @brief Destroys a command table
void tfs_command_table_destroy(TfsCommandTable* self);

/// @brief Pushes a command into this command table atomically
TfsCommandTablePushResult tfs_command_table_push(TfsCommandTable* self, TfsCommand command);

/// @brief Pops a command from this command table atomically.
TfsCommandTablePopResult tfs_command_table_pop(TfsCommandTable* self);

#endif
