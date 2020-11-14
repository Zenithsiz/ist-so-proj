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
#include <tfs/cond_var.h>		 // TfsCondVar
#include <tfs/mutex.h>			 // TfsMutex

/// @brief The command table
/// @details
/// An expandable ring-buffer of commands that
/// acts as a single-producer-multiple-consumer.
typedef struct TfsCommandTable {
	/// @brief All of the commands
	TfsCommand* commands;

	/// @brief Size of the table
	size_t size;

	/// @brief Index of first command
	size_t first_idx;

	/// @brief Index of last command, non-inclusive
	size_t last_idx;

	/// @brief If the writer has exited.
	bool writer_exited;

	/// @brief Mutex for synchronization.
	TfsMutex mutex;

	/// @brief Condition variable for reader to wait on
	TfsCondVar reader_cond_var;

	/// @brief Condition variable for writer to wait on
	TfsCondVar writer_cond_var;
} TfsCommandTable;

/// @brief Creates a new, empty, command table
/// @param size Size for the ring buffer
TfsCommandTable tfs_command_table_new(size_t size);

/// @brief Destroys a command table
void tfs_command_table_destroy(TfsCommandTable* self);

/// @brief Pushes a command into this command table
/// @param self
/// @param command The command to push
void tfs_command_table_push(TfsCommandTable* self, TfsCommand command);

/// @brief Signals all current and future readers that the writer has left and
///        for them to quit.
void tfs_command_table_writer_exit(TfsCommandTable* self);

/// @brief Pops a command from this command table
/// @details
/// This will wait for a writer to push a command onto the table.
/// If the command
/// @param self
/// @param[out] command The command popped
/// @return Returns false if the
bool tfs_command_table_pop(TfsCommandTable* self, TfsCommand* command);

#endif
