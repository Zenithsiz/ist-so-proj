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

TfsCommandTable* tfs_command_table_new(TfsLockKind lock_kind) {
	TfsCommandTable* table = malloc(1 * sizeof(TfsCommandTable));
	table->first_idx	   = 0;
	table->last_idx		   = 0;
	table->lock			   = tfs_lock_new(lock_kind);

	return table;
}

void tfs_command_table_destroy(TfsCommandTable* self) {
	for (size_t n = self->first_idx; n < self->last_idx; n++) {
		tfs_command_destroy(&self->commands[n]);
	}

	tfs_lock_destroy(&self->lock);
	free(self);
}

TfsCommandTablePushResult tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// Lock and check if we have enough capacity
	tfs_lock_write_lock(&self->lock);
	if (self->last_idx == TFS_COMMAND_TABLE_MAX) {
		tfs_lock_unlock(&self->lock);
		return TfsCommandTablePushResultErrorFull;
	}

	// Add the command at the end
	self->commands[self->last_idx] = command;
	self->last_idx++;

	// Unlock and return success
	tfs_lock_unlock(&self->lock);
	return TfsCommandTablePushResultSuccess;
}

TfsCommandTablePopResult tfs_command_table_pop(TfsCommandTable* self) {
	// Lock and check if we have any commands
	tfs_lock_write_lock(&self->lock);
	if (self->first_idx == self->last_idx) {
		tfs_lock_unlock(&self->lock);
		return (TfsCommandTablePopResult){.is_some = false};
	}

	// Get our first command
	TfsCommand command = self->commands[self->first_idx];
	self->first_idx++;

	// Unlock and return the command
	tfs_lock_unlock(&self->lock);
	return (TfsCommandTablePopResult){.is_some = true, .data = {.command = command}};
}
