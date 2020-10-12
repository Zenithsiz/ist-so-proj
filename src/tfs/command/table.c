#include "table.h"

// Imports
#include <stdlib.h> // malloc, free

TfsCommandTable* tfs_command_table_new(void) {
	TfsCommandTable* table = malloc(1 * sizeof(TfsCommandTable));
	table->first_idx	   = 0;
	table->last_idx		   = 0;

	return table;
}

void tfs_command_table_destroy(TfsCommandTable* self) {
	for (size_t n = self->first_idx; n < self->last_idx; n++) {
		tfs_command_destroy(&self->commands[n]);
	}

	free(self);
}

bool tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// If we don't have enough capacity, return Err
	if (self->last_idx == TFS_COMMAND_TABLE_MAX) {
		return false;
	}

	// Add the command at the end
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
