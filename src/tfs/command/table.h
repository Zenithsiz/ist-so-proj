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
#include <tfs/lock.h>			 // TfsCommandTableLock

/// @brief Max number of commands
#define TFS_COMMAND_TABLE_MAX 150000

/// @brief The command table
/// @details
/// This table is specifically designed to be
/// filled once and then emptied, with no further
/// pushes.
typedef struct TfsCommandTable {
	/// @brief All of the commands
	TfsCommand commands[TFS_COMMAND_TABLE_MAX];

	/// @brief Index of first command
	size_t first_idx;

	/// @brief Index of last command, non-inclusive
	size_t last_idx;

	/// @brief Lock
	TfsLock lock;
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
TfsCommandTable* tfs_command_table_new(TfsLockKind lock_kind);

/// @brief Destroys a command table
void tfs_command_table_destroy(TfsCommandTable* self);

/// @brief Pushes a command into this command table atomically
TfsCommandTablePushResult tfs_command_table_push(TfsCommandTable* self, TfsCommand command);

/// @brief Pops a command from this command table atomically.
TfsCommandTablePopResult tfs_command_table_pop(TfsCommandTable* self);

#endif
