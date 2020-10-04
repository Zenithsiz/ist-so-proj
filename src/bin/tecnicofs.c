#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tfs/fs.h>

#define MAX_INPUT_SIZE 100

static void apply_commands(TfsFs* fs) {
	char line[MAX_INPUT_SIZE];

	while (fgets(line, sizeof(line), stdin)) {
		char token, type;
		char name[MAX_INPUT_SIZE];

		int numTokens = sscanf(line, "%c %s %c", &token, name, &type);
		if (numTokens < 2) {
			fprintf(stderr, "Error: invalid command\n");
			return;
		}

		TfsPath path = tfs_path_from_cstr(name);

		switch (token) {
			case 'c':
				switch (type) {
					case 'f': {
						printf("Creating file %s\n", name);
						TfsFsCreateResult res = tfs_fs_create(fs, TfsInodeTypeFile, path);
						if (res.kind != TfsFsCreateResultSuccess) {
							printf("Unable to create file '%s'\n", name);
							tfs_fs_create_result_print(&res, stdout);
						}
						else {
							printf("Successfully created '%s'\n", name);
						}
						break;
					}
					case 'd': {
						printf("Creating directory '%s'\n", name);
						TfsFsCreateResult res = tfs_fs_create(fs, TfsInodeTypeDir, path);
						if (res.kind != TfsFsCreateResultSuccess) {
							printf("Unable to create directory '%s'\n", name);
							tfs_fs_create_result_print(&res, stdout);
						}
						else {
							printf("Successfully created '%s' (with index %zu)\n", name, res.data.success.idx);
						}
						break;
					}
					default: {
						fprintf(stderr, "Error: invalid inode type: '%c'\n", type);
						continue;
					}
				}
				break;
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
				fprintf(stderr, "Unknown command: '%c'\n", token);
				break;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	/* init filesystem */
	TfsFs fs = tfs_fs_new();

	/* process input and print tree */
	apply_commands(&fs);
	tfs_fs_print(&fs, stdout);

	/* release allocated memory */
	tfs_fs_destroy(&fs);

	return EXIT_SUCCESS;
}
