#include "table.h"

// Imports
#include <stdlib.h> // malloc, free

void tfs_command_table_push_result_print(const TfsCommandTablePushResult* self, FILE* out) {
	switch (*self) {
		case TfsCommandTablePushResultErrorFull: {
			fprintf(out, "Command table is full\n");
			break;
		}

		case TfsCommandTablePushResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

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

TfsCommandTablePushResult tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// If we don't have enough capacity, return Err
	if (self->last_idx == TFS_COMMAND_TABLE_MAX) {
		return TfsCommandTablePushResultErrorFull;
	}

	// Add the command at the end
	self->commands[self->last_idx] = command;
	self->last_idx++;

	return TfsCommandTablePushResultSuccess;
}

TfsCommandTablePopResult tfs_command_table_pop(TfsCommandTable* self) {
	// If we don't have any commands, return Err
	if (self->first_idx == self->last_idx) {
		return (TfsCommandTablePopResult){.is_some = false};
	}

	// Get our first command
	TfsCommand command = self->commands[self->first_idx];
	self->first_idx++;

	return (TfsCommandTablePopResult){.is_some = true, .data = {.command = command}};
}
