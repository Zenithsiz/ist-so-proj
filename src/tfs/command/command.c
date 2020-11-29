#include "command.h"

// Includes
#include <stdlib.h> // free
#include <string.h> // strlen

void tfs_command_parse_error_print(const TfsCommandParseError* self, FILE* out) {
	switch (self->kind) {
		case TfsCommandParseErrorReadLine: {
			fprintf(out, "Unable to read line from `in`\n");
			break;
		}
		case TfsCommandParseErrorNoCommand: {
			fprintf(out, "Missing command from line\n");
			break;
		}
		case TfsCommandParseErrorInvalidCommand: {
			fprintf(out, "Invalid command type: '%c'\n", self->data.invalid_command.command);
			break;
		}
		case TfsCommandParseErrorMissingCreateArgs: {
			fprintf(out, "Missing arguments for `Create` command\n");
			break;
		}
		case TfsCommandParseErrorInvalidType:
			fprintf(out, "Invalid type for `Create` command: '%c'\n", self->data.invalid_type.type);
			break;
		case TfsCommandParseErrorMissingSearchArgs: {
			fprintf(out, "Missing arguments for `Search` command\n");
			break;
		}
		case TfsCommandParseErrorMissingRemoveArgs: {
			fprintf(out, "Missing arguments for `Remove` command\n");
			break;
		}
		case TfsCommandParseErrorMissingMoveArgs: {
			fprintf(out, "Missing arguments for `Move` command\n");
			break;
		}
		case TfsCommandParseErrorMissingPrintArgs: {
			fprintf(out, "Missing arguments for `Print` command\n");
			break;
		}
		default: {
			break;
		}
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
		// Print path
		// m <path>
		case 'p': {
			if (tokens_read != 2) {
				return (TfsCommandParseResult){
					.success = false,
					.data.err.kind = TfsCommandParseErrorMissingPrintArgs,
				};
			}

			char* path = strdup(args[0]);
			return (TfsCommandParseResult){
				.success = true,
				.data.command.kind = TfsCommandPrint,
				.data.command.data.print.path = path,
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

void tfs_command_to_string(const TfsCommand* command, char* buffer, size_t buffer_len) {
	switch (command->kind) {
		case TfsCommandCreate: {
			snprintf(buffer,
				buffer_len,
				"c %.*s %c",
				(int)command->data.create.path.len,
				command->data.create.path.chars,
				command->data.create.type == TfsInodeTypeFile ? 'f' : 'd' //
			);
			break;
		}
		case TfsCommandSearch: {
			snprintf(buffer, buffer_len, "l %.*s", (int)command->data.search.path.len, command->data.search.path.chars);
			break;
		}
		case TfsCommandRemove: {
			snprintf(buffer, buffer_len, "d %.*s", (int)command->data.remove.path.len, command->data.remove.path.chars);
			break;
		}
		case TfsCommandMove: {
			snprintf(buffer,
				buffer_len,
				"m %.*s %.*s",
				(int)command->data.move.source.len,
				command->data.move.source.chars,
				(int)command->data.move.dest.len,
				command->data.move.dest.chars //
			);
			break;
		}
		case TfsCommandPrint: {
			snprintf(buffer, buffer_len, "p %s", command->data.print.path);
			break;
		}
		default: {
			break;
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
		case TfsCommandPrint: {
			free(command->data.print.path);
			break;
		}

		default: {
			break;
		}
	}
}
