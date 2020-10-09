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

TfsCommandTable* tfs_command_table_new(TfsCommandTableLockKind lock_kind) {
	TfsCommandTable* table = malloc(1 * sizeof(TfsCommandTable));
	table->len			   = 0;
	table->lock			   = tfs_command_table_lock_new(lock_kind);

	return table;
}

void tfs_command_table_destroy(TfsCommandTable* self) {
	for (size_t n = 0; n < self->len; n++) {
		tfs_command_destroy(&self->commands[n]);
	}

	tfs_command_table_lock_destroy(&self->lock);
	free(self);
}

TfsCommandTablePushResult tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// Lock and check if we have enough capacity
	tfs_command_table_lock_write_lock(&self->lock);
	if (self->len == TFS_COMMAND_TABLE_MAX) {
		tfs_command_table_lock_unlock(&self->lock);
		return TfsCommandTablePushResultErrorFull;
	}

	// Add the command at the end and increase our length
	self->commands[self->len] = command;
	self->len++;

	// Unlock and return success
	tfs_command_table_lock_unlock(&self->lock);
	return TfsCommandTablePushResultSuccess;
}

TfsCommandTablePopResult tfs_command_table_pop(TfsCommandTable* self) {
	// Lock and check if we have any commands
	tfs_command_table_lock_write_lock(&self->lock);
	if (self->len == 0) {
		tfs_command_table_lock_unlock(&self->lock);
		return (TfsCommandTablePopResult){.is_some = false};
	}

	// Get the last command and decrease our length
	TfsCommand command = self->commands[self->len - 1];
	self->len--;

	// Unlock and return the command
	tfs_command_table_lock_unlock(&self->lock);
	return (TfsCommandTablePopResult){.is_some = true, .data = {.command = command}};
}
