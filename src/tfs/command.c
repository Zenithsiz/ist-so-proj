#include "command.h"

TfsCommandParseResult tfs_command_parse(FILE* in) {
	char command;
	char path_cstr[1024];
	char type;

	int tokens_read = fscanf(in, " %c %1024s %c \n", &command, path_cstr, &type);
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
			// If we didn't get a type, return Err
			if (tokens_read != 3) {
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
						.kind = TfsCommandParseResultErrorInvalidType};
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
				.kind = TfsCommandParseResultErrorInvalidCommand};
		}
	}
}

void tfs_command_destroy(TfsCommand command) {
	switch (command.kind) {
		case TfsCommandCreate: {
			tfs_path_owned_destroy(command.data.create.path);
			break;
		}

		case TfsCommandSearch: {
			tfs_path_owned_destroy(command.data.search.path);
			break;
		}

		case TfsCommandRemove: {
			tfs_path_owned_destroy(command.data.remove.path);
			break;
		}

		default: {
			break;
		}
	}
}
