#include "command.h"

void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out) {
	switch (self->kind) {
		case TfsCommandParseErrorNoCommand: {
			fprintf(out, "No command was supplied\n");
			break;
		}

		case TfsCommandParseErrorNoPath: {
			fprintf(out, "No path was supplied\n");
			break;
		}

		case TfsCommandParseErrorNoType: {
			fprintf(out, "No type was supplied\n");
			break;
		}

		case TfsCommandParseErrorInvalidCommand: {
			fprintf(out, "Invalid command '%c'\n", self->data.invalid_command.command);
			break;
		}

		case TfsCommandParseErrorInvalidType: {
			fprintf(out, "Invalid type '%c'\n", self->data.invalid_type.type);
			break;
		}

		default: {
			break;
		}
	}
}

bool tfs_command_parse(FILE* in, TfsCommand* command, TfsCommandParseError* err) {
	// Read the command and path
	char command_char;
	char path_cstr[1024];
	int tokens_read = fscanf(in, " %c %1024s", &command_char, path_cstr);
	if (tokens_read <= 0) {
		if (err != NULL) {
			*err = (TfsCommandParseError){
				.kind = TfsCommandParseErrorNoCommand,
			};
		}
		return false;
	}
	if (tokens_read == 1) {
		if (err != NULL) {
			*err = (TfsCommandParseError){
				.kind = TfsCommandParseErrorNoPath,
			};
		}
		return false;
	}

	// And check the command
	switch (command_char) {
		// Create path
		case 'c': {
			// Read the type
			char type;
			tokens_read = fscanf(in, " %c", &type);
			if (tokens_read != 1) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorNoType,
					};
				}
				return false;
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
					if (err != NULL) {
						*err = (TfsCommandParseError){
							.kind = TfsCommandParseErrorInvalidType,
							.data = {.invalid_type = {.type = type}},
						};
					}
					return false;
				}
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			if (command != NULL) {
				command->kind			  = TfsCommandCreate;
				command->data.create.path = path;
				command->data.create.type = inode_type;
			}
			return true;
		}

		// Look up path
		case 'l': {
			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			if (command != NULL) {
				command->kind			  = TfsCommandSearch;
				command->data.search.path = path;
			}
			return true;
		}

		// Delete path
		case 'd': {
			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(path_cstr));
			if (command != NULL) {
				command->kind			  = TfsCommandRemove;
				command->data.remove.path = path;
			}
			return true;
		}
		default: {
			if (err != NULL) {
				*err = (TfsCommandParseError){
					.kind = TfsCommandParseErrorInvalidCommand,
					.data = {.invalid_command = {.command = command_char}},
				};
			}
			return false;
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
