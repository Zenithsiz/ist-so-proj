#include "command.h"

void tfs_command_parse_result_print(const TfsCommandParseResult* self, FILE* out) {
	switch (self->kind) {
		case TfsCommandParseResultErrorNoCommand: {
			fprintf(out, "No command was supplied\n");
			break;
		}

		case TfsCommandParseResultErrorNoPath: {
			fprintf(out, "No path was supplied\n");
			break;
		}

		case TfsCommandParseResultErrorNoType: {
			fprintf(out, "No type was supplied\n");
			break;
		}

		case TfsCommandParseResultErrorInvalidCommand: {
			fprintf(out, "Invalid command '%c'\n", self->data.invalid_command.command);
			break;
		}

		case TfsCommandParseResultErrorInvalidType: {
			fprintf(out, "Invalid type '%c'\n", self->data.invalid_type.type);
			break;
		}

		case TfsCommandParseResultSuccess:
		default:
			fprintf(out, "Success\n");
			break;
	}
}

TfsCommandParseResult tfs_command_parse(FILE* in) {
	// Read the command and path
	char command;
	char path_cstr[1024];
	int tokens_read = fscanf(in, " %c %1024s", &command, path_cstr);
	if (tokens_read <= 0) {
		return (TfsCommandParseResult){
			.kind = TfsCommandParseResultErrorNoCommand,
		};
	}
	if (tokens_read == 1) {
		return (TfsCommandParseResult){
			.kind = TfsCommandParseResultErrorNoPath,
		};
	}

	// And check the command
	switch (command) {
		// Create path
		case 'c': {
			// Read the type
			char type;
			tokens_read = fscanf(in, " %c", &type);
			if (tokens_read != 1) {
				return (TfsCommandParseResult){
					.kind = TfsCommandParseResultErrorNoType,
				};
			}

			// Get the type of what we're creating
			TfsInodeType inode_type;
			switch (type) {
				case 'f': {
					inode_type = TfsInodeTypeFile;
					break;
				}
				case 'd': {
					inode_type = TfsInodeTypeDir;
					break;
				}
				default: {
					return (TfsCommandParseResult){
						.kind = TfsCommandParseResultErrorInvalidType,
						.data = {.invalid_type = {.type = type}}};
				}
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			return (TfsCommandParseResult){
				.kind = TfsCommandParseResultSuccess,
				.data = {.success = {.command = {.kind = TfsCommandCreate, .data = {.create = {.path = path, .type = inode_type}}}}}};
		}

		// Look up path
		case 'l': {
			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			return (TfsCommandParseResult){
				.kind = TfsCommandParseResultSuccess,
				.data = {.success = {.command = {.kind = TfsCommandSearch, .data = {.search = {.path = path}}}}}};
		}

		// Delete path
		case 'd': {
			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			return (TfsCommandParseResult){
				.kind = TfsCommandParseResultSuccess,
				.data = {.success = {.command = {.kind = TfsCommandSearch, .data = {.remove = {.path = path}}}}}};
		}
		default: {
			return (TfsCommandParseResult){
				.kind = TfsCommandParseResultErrorInvalidCommand,
				.data = {.invalid_command = {.command = command}}};
		}
	}
}

void tfs_command_destroy(TfsCommand* command) {
	switch (command->kind) {
		case TfsCommandCreate: {
			tfs_path_owned_destroy(&command->data.create.path);
			break;
		}

		case TfsCommandSearch: {
			tfs_path_owned_destroy(&command->data.search.path);
			break;
		}

		case TfsCommandRemove: {
			tfs_path_owned_destroy(&command->data.remove.path);
			break;
		}

		default: {
			break;
		}
	}
}
