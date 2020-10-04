#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tfs/fs.h>

/// @brief Applies commands from stdin
static void apply_commands(TfsFs* fs) {
	// Each command has at most `[command] [name] {type}\n`
	// `command` / `type`: 1 character,
	// `name`: TFS_DIR_MAX_FILE_NAME_LEN characters + 1 null.
	char line[TFS_DIR_MAX_FILE_NAME_LEN + 6];

	while (fgets(line, sizeof(line), stdin)) {
		// If the line starts with ^D, return
		if (line[0] == '\x4') {
			return;
		}

		// Read at least the command and name
		char command, type;
		char name[TFS_DIR_MAX_FILE_NAME_LEN + 1];
		int tokens_read = sscanf(line, "%c %s %c\n", &command, name, &type);
		if (tokens_read < 2) {
			fprintf(stderr, "Usage: [command] [name] {type}\n");
			continue;
		}

		// Create a path from the name
		TfsPath path = tfs_path_from_cstr(name);

		// And check the command
		switch (command) {
			// Create path
			case 'c': {
				// If we didn't get a type, print err and continue
				if (tokens_read != 3) {
					fprintf(stderr, "Usage: c [name] [type]\n");
					continue;
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
						fprintf(stderr, "Invalid inode type: '%c'\n", type);
						continue;
					}
				}

				printf("Creating %s %s\n", tfs_inode_type_str(inode_type), name);
				TfsFsCreateResult res = tfs_fs_create(fs, inode_type, path);
				if (res.kind != TfsFsCreateResultSuccess) {
					printf("Unable to create %s '%s'\n", tfs_inode_type_str(inode_type), name);
					tfs_fs_create_result_print(&res, stdout);
				}
				else {
					printf("Successfully created '%s'\n", name);
				}

				break;
			}

			// Look up path
			case 'l': {
				TfsFsFindResult res = tfs_fs_find(fs, path);
				if (res.kind != TfsFsFindResultSuccess) {
					printf("Unable to find %s\n", name);
					tfs_fs_find_result_print(&res, stdout);
				}
				else {
					printf("Found %s\n", name);
				}
				break;
			}

			// Delete path
			case 'd': {
				printf("Delete: %s\n", name);
				TfsFsRemoveResult res = tfs_fs_remove(fs, path);
				if (res.kind != TfsFsRemoveResultSuccess) {
					printf("Unable to delete %s\n", name);
					tfs_fs_remove_result_print(&res, stdout);
				}
				else {
					printf("Successfully deleted %s (with index %zu)\n", name, res.data.success.idx);
				}
				break;
			}
			default: {
				fprintf(stderr, "Unknown command: '%c'\n", command);
				break;
			}
		}
	}
}

int main(void) {
	// Create the file system
	TfsFs fs = tfs_fs_new();

	// Process all input
	apply_commands(&fs);

	// Print the tree before exiting
	tfs_fs_print(&fs, stdout);

	// And destroy the file system.
	tfs_fs_destroy(&fs);

	return EXIT_SUCCESS;
}
