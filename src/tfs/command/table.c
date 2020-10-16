#include "table.h"

// Imports
#include <stdlib.h>	  // malloc, free
#include <tfs/util.h> // tfs_clamp_size_t

/// @brief Reallocates the command table.
/// @details
/// If unable to reallocate the table, this function
/// will call `exit` with `EXIT_FAILURE`.
static void tfs_command_table_realloc(TfsCommandTable* self) {
	// Double the current capacity so we don't allocate often
	// Note: We allocate at least 4 because `2 * 0 == 0`.
	size_t new_capacity = tfs_clamp_size_t(2 * self->capacity, 4, TFS_COMMAND_TABLE_MAX);

	// Try to allocate
	// Note: We can pass `NULL` to `realloc`.
	TfsCommand* new_commands = realloc(self->commands, new_capacity * sizeof(TfsCommand));
	if (new_commands == NULL) {
		fprintf(stderr, "Unable to expand command table capacity to %zu\n", new_capacity);
		exit(EXIT_FAILURE);
	}

	// Note: No need to initialize new commands.

	// Move everything into `self`
	self->commands = new_commands;
	self->capacity = new_capacity;
}

TfsCommandTable tfs_command_table_new(void) {
	return (TfsCommandTable){
		.commands  = NULL,
		.capacity  = 0,
		.first_idx = 0,
		.last_idx  = 0,
	};
}

void tfs_command_table_destroy(TfsCommandTable* self) {
	for (size_t n = self->first_idx; n < self->last_idx; n++) {
		tfs_command_destroy(&self->commands[n]);
	}

	free(self->commands);
}

bool tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// If we're full, try to reallocate
	if (self->last_idx == self->capacity) {
		// If we reached the maximum size already, just return false
		// instead of reallocating
		if (self->capacity >= TFS_COMMAND_TABLE_MAX) {
			return false;
		}

		// Else just reallocate and continue
		tfs_command_table_realloc(self);
	}

	// Add the command at the end
	// Note: Since we reallocate if `self-last_idx` is
	//       at the end, this index will always be valid.
	self->commands[self->last_idx] = command;
	self->last_idx++;

	return true;
}

bool tfs_command_table_pop(TfsCommandTable* self, TfsCommand* command) {
	// If we don't have any commands, return Err
	if (self->first_idx == self->last_idx) {
		return false;
	}

	// Get our first command
	if (command != NULL) {
		*command = self->commands[self->first_idx];
	}
	self->first_idx++;

	return true;
}
