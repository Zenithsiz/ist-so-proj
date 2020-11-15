#include "table.h"

// Imports
#include <stdlib.h>	  // malloc, free
#include <tfs/util.h> // tfs_max_size_t

TfsCommandTable tfs_command_table_new(size_t size) {
	// Try to allocate
	// Note: We can pass `NULL` to `realloc`.
	TfsCommand* commands = malloc(size * sizeof(TfsCommand));
	if (commands == NULL) {
		fprintf(stderr, "Unable to allocate command table with %zu elements\n", size);
		exit(EXIT_FAILURE);
	}

	return (TfsCommandTable){
		.commands		 = commands,
		.size			 = size,
		.first_idx		 = 0,
		.last_idx		 = 0,
		.writer_exited	 = false,
		.mutex			 = tfs_mutex_new(),
		.reader_cond_var = tfs_cond_var_new(),
		.writer_cond_var = tfs_cond_var_new(),
	};
}

void tfs_command_table_destroy(TfsCommandTable* self) {
	// Free any leftover commands.
	while (self->first_idx != self->last_idx) {
		TfsCommand command;
		tfs_command_table_pop(self, &command);
		tfs_command_destroy(&command);
	}

	// Free the commands buffer
	free(self->commands);

	// Then the mutex and cond vars
	tfs_mutex_destroy(&self->mutex);
	tfs_cond_var_destroy(&self->reader_cond_var);
	tfs_cond_var_destroy(&self->writer_cond_var);
}

void tfs_command_table_push(TfsCommandTable* self, TfsCommand command) {
	// Lock the table
	tfs_mutex_lock(&self->mutex);

	// If we're 1 command behind the reader, we're full, so wait
	// Note: We do this wait when we're 1 behind and not when we're
	//       on the same element, so we can distinguish between empty
	//       and full capacity.
	while (self->last_idx == (self->first_idx + 1) % self->size) {
		tfs_cond_var_wait(&self->writer_cond_var, &self->mutex);
	}

	// Then add the command at the end
	self->commands[self->last_idx] = command;
	self->last_idx				   = (self->last_idx + 1) % self->size;

	// Unlock and wake up any readers waiting
	tfs_cond_var_signal(&self->reader_cond_var);
	tfs_mutex_unlock(&self->mutex);
}

void tfs_command_table_writer_exit(TfsCommandTable* self) {
	// Lock the table
	tfs_mutex_lock(&self->mutex);

	// Signal that the writer has exited.
	self->writer_exited = true;

	// Unlock and wake up all readers waiting
	tfs_cond_var_broadcast(&self->reader_cond_var);
	tfs_mutex_unlock(&self->mutex);
}

bool tfs_command_table_pop(TfsCommandTable* self, TfsCommand* command) {
	// Lock the table
	tfs_mutex_lock(&self->mutex);

	// While we're empty
	while (self->last_idx == self->first_idx) {
		// If the writer has exited, return false
		if (self->writer_exited) {
			tfs_mutex_unlock(&self->mutex);
			return false;
		}

		// Else wait
		tfs_cond_var_wait(&self->reader_cond_var, &self->mutex);
	}

	// Get our command
	if (command != NULL) {
		*command = self->commands[self->first_idx];
	}
	self->first_idx = (self->first_idx + 1) % self->size;

	// Unlock and wake the writer waiting
	tfs_cond_var_signal(&self->writer_cond_var);
	tfs_mutex_unlock(&self->mutex);

	return true;
}
