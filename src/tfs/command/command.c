#include "command.h"

// Includes
#include <string.h> // strlen

void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out) {
	switch (self->kind) {
		case TfsCommandParseErrorReadLine: fprintf(out, "Unable to read line from `in`\n"); break;
		case TfsCommandParseErrorNoCommand: fprintf(out, "Missing command from line\n"); break;
		case TfsCommandParseErrorInvalidCommand:
			fprintf(out, "Invalid command type: '%c'\n", self->data.invalid_command.command);
			break;
		case TfsCommandParseErrorMissingCreateArgs: fprintf(out, "Missing arguments for `Create` command\n"); break;
		case TfsCommandParseErrorInvalidType:
			fprintf(out, "Invalid type for `Create` command: '%c'\n", self->data.invalid_type.type);
			break;
		case TfsCommandParseErrorMissingSearchArgs: fprintf(out, "Missing arguments for `Search` command\n"); break;
		case TfsCommandParseErrorMissingRemoveArgs: fprintf(out, "Missing arguments for `Remove` command\n"); break;
		case TfsCommandParseErrorMissingMoveArgs: fprintf(out, "Missing arguments for `Move` command\n"); break;
		default: break;
	}
}

TfsCommandParseResult tfs_command_parse(FILE* in) {
	// Read a line
	char line[1024];
	if (fgets(line, sizeof(line), in) == NULL) {
		return (TfsCommandParseResult){
			.success = false,
			.data.err.kind = TfsCommandParseErrorReadLine,
		};
	}

	// Read the arguments
	char command_char;
	char args[2][1024];
	int tokens_read = sscanf(line, " %c %1023s %1023s", &command_char, args[0], args[1]);
	if (tokens_read < 1) {
		return (TfsCommandParseResult){
			.success = false,
			.data.err.kind = TfsCommandParseErrorNoCommand,
		};
	}

	switch (command_char) {
		// Create path
		// c <path> <inode-type>
		case 'c': {
			if (tokens_read != 3) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorMissingCreateArgs,
				};
			}

			// Get the type of what we're creating
			TfsInodeType inode_type;
			size_t type_len = strlen(args[1]);
			if (type_len != 1) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorInvalidType,
					.data.err.data.invalid_type.type = '\0',
					.data.err.data.invalid_type.len = type_len,
				};
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
					return (TfsCommandParseResult){
						.success = false,
						.data.err.kind = TfsCommandParseErrorInvalidType,
						.data.err.data.invalid_type.type = args[1][0],
						.data.err.data.invalid_type.len = 1,
					};
				}
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			return (TfsCommandParseResult){
				.success = true,
				.data.command.kind = TfsCommandCreate,
				.data.command.data.create.path = path,
				.data.command.data.create.type = inode_type,
			};
		}

		// Look up path
		// l <path>
		case 'l': {
			if (tokens_read != 2) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorMissingSearchArgs,
				};
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			return (TfsCommandParseResult){
				.success = true,
				.data.command.kind = TfsCommandSearch,
				.data.command.data.search.path = path,
			};
		}

		// Remove path
		// d <path>
		case 'd': {
			if (tokens_read != 2) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorMissingRemoveArgs,
				};
			}

			TfsPathOwned path = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			return (TfsCommandParseResult){
				.success = true,
				.data.command.kind = TfsCommandRemove,
				.data.command.data.remove.path = path,
			};
		}

		// Move path
		// m <source> <dest>
		case 'm': {
			if (tokens_read != 3) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorMissingMoveArgs,
				};
			}

			TfsPathOwned source = tfs_path_to_owned(tfs_path_from_cstr(args[0]));
			TfsPathOwned dest = tfs_path_to_owned(tfs_path_from_cstr(args[1]));
			return (TfsCommandParseResult){
				.success = true,
				.data.command.kind = TfsCommandMove,
				.data.command.data.move.source = source,
				.data.command.data.move.dest = dest,
			};
		}
		default: {
			return (TfsCommandParseResult){
				.success = false,
				.data.command.kind = TfsCommandParseErrorInvalidCommand,
				.data.err.data.invalid_command.command = command_char,
			};
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
