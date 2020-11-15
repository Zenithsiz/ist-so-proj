#include "command.h"

// Includes
#include <string.h> // strlen

void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out) {
	switch (self->kind) {
		case TfsCommandParseErrorReadLine:
			fprintf(out, "Unable to read line from `in`\n");
			break;
		case TfsCommandParseErrorNoCommand:
			fprintf(out, "Missing command from line\n");
			break;
		case TfsCommandParseErrorInvalidCommand:
			fprintf(out, "Invalid command type: '%c'\n", self->data.invalid_command.command);
			break;
		case TfsCommandParseErrorMissingCreateArgs:
			fprintf(out, "Missing arguments for `Create` command\n");
			break;
		case TfsCommandParseErrorInvalidType:
			fprintf(out, "Invalid type for `Create` command: '%c'\n", self->data.invalid_type.type);
			break;
		case TfsCommandParseErrorMissingSearchArgs:
			fprintf(out, "Missing arguments for `Search` command\n");
			break;
		case TfsCommandParseErrorMissingRemoveArgs:
			fprintf(out, "Missing arguments for `Remove` command\n");
			break;
		case TfsCommandParseErrorMissingMoveArgs:
			fprintf(out, "Missing arguments for `Move` command\n");
			break;
		default:
			break;
	}
}

bool tfs_command_parse(FILE* in, TfsCommand* command, TfsCommandParseError* err) {
	// Read a line
	char line[1024];
	if (fgets(line, sizeof(line), in) == NULL) {
		if (err != NULL) {
			*err = (TfsCommandParseError){
				.kind = TfsCommandParseErrorReadLine,
			};
		}
		return false;
	}

	// Read the arguments
	char command_char;
	char args[2][1024];
	int tokens_read = sscanf(line, " %c %1023s %1023s", &command_char, args[0], args[1]);
	if (tokens_read < 1) {
		if (err != NULL) {
			*err = (TfsCommandParseError){
				.kind = TfsCommandParseErrorNoCommand,
			};
		}
		return false;
	}

	switch (command_char) {
		// Create path
		// c <path> <inode-type>
		case 'c': {
			if (tokens_read != 3) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorMissingCreateArgs,
					};
				}
				return false;
			}

			// Get the type of what we're creating
			TfsInodeType inode_type;
			size_t type_len = strlen(args[1]);
			if (type_len != 1) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorInvalidType,
						.data = {.invalid_type = {.type = '\0', .len = type_len}},
					};
				}
				return false;
			}
			switch (args[1][0]) {
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
							.data = {.invalid_type = {.type = args[1][0], .len = 1}},
						};
					}
					return false;
				}
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			if (command != NULL) {
				command->kind			  = TfsCommandCreate;
				command->data.create.path = path;
				command->data.create.type = inode_type;
			}
			return true;
		}

		// Look up path
		// l <path>
		case 'l': {
			if (tokens_read != 2) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorMissingSearchArgs,
					};
				}
				return false;
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			if (command != NULL) {
				command->kind			  = TfsCommandSearch;
				command->data.search.path = path;
			}
			return true;
		}

		// Remove path
		// d <path>
		case 'd': {
			if (tokens_read != 2) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorMissingRemoveArgs,
					};
				}
				return false;
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			if (command != NULL) {
				command->kind			  = TfsCommandRemove;
				command->data.remove.path = path;
			}
			return true;
		}

		// Move path
		// m <source> <dest>
		case 'm': {
			if (tokens_read != 3) {
				if (err != NULL) {
					*err = (TfsCommandParseError){
						.kind = TfsCommandParseErrorMissingSearchArgs,
					};
				}
				return false;
			}

			TfsPathOwned source = tfs_path_to_owned(tfs_path_from_cstr(args[1]));
			TfsPathOwned dest	= tfs_path_to_owned(tfs_path_from_cstr(args[2]));
			if (command != NULL) {
				command->kind			  = TfsCommandMove;
				command->data.move.source = source;
				command->data.move.dest	  = dest;
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

		case TfsCommandMove: {
			tfs_path_owned_destroy(&command->data.move.source);
			tfs_path_owned_destroy(&command->data.move.dest);
			break;
		}

		default: {
			break;
		}
	}
}
