/// @file
/// @brief Command table
/// @details
/// This file defines the #TfsCommandTable type,
/// which is responsible for holding all commands
/// for the filesystem to execute.

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
/// An expandable ring-buffer of commands.
/// @warning
/// All methods of this type are _not_ thread-safe
/// and should use an external lock for synchronization.
/// @note
/// For the first exercice, this has a limit of @ref TFS_COMMAND_TABLE_MAX
/// commands, which, once reached, will stop accepting new commands. This
/// makes the 'ring' part of the buffer currently useless, but for the second
/// exercice the indices will wrap around back to the beginning of the buffer
/// to fully implement the 'ring' part of the buffer.
typedef struct TfsCommandTable {
	/// @brief All of the commands
	TfsCommand* commands;

	/// @brief Buffer capacity
	size_t capacity;

	/// @brief Index of first command
	size_t first_idx;

	/// @brief Index of last command, non-inclusive
	size_t last_idx;
} TfsCommandTable;

/// @brief Creates a new, empty, command table
TfsCommandTable tfs_command_table_new(void);

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
