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
} TfsCommandTable;

/// @brief Creates a new, empty, command table
/// @details
/// Result is allocated to prevent stack smashing
TfsCommandTable* tfs_command_table_new(void);

/// @brief Destroys a command table
void tfs_command_table_destroy(TfsCommandTable* self);

/// @brief Pushes a command into this command table
/// @param self
/// @param command The command to push
/// @return If successfully pushed.
bool tfs_command_table_push(TfsCommandTable* self, TfsCommand command);

/// @brief Pops a command from this command table
/// @param self
/// @param[out] command The command popped
/// @return If successfully popped.
bool tfs_command_table_pop(TfsCommandTable* self, TfsCommand* command);

#endif
