#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tfs/tfs.h>

#define MAX_INPUT_SIZE 100

static void apply_commands(TfsFileSystem* fs) {
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

		TfsInodeIdx searchResult;
		switch (token) {
			case 'c':
				switch (type) {
					case 'f':
						printf("Create file: %s\n", name);
						if (tfs_create_inode(fs, TfsInodeTypeFile, path) != TfsFileSystemErrorSuccess)
							printf("Create: could not create file %s\n", name);
						else
							printf("Create: %s successfully created\n", name);
						break;
					case 'd':
						printf("Create directory: %s\n", name);
						if (tfs_create_inode(fs, TfsInodeTypeDir, path) != TfsFileSystemErrorSuccess)
							printf("Create: could not create directory %s\n", name);
						else
							printf("Create: %s successfully created\n", name);
						break;
					default:
						fprintf(stderr, "Error: invalid node type\n");
						continue;
				}
				break;
			case 'l':
				if (tfs_find(fs, path, &searchResult) != TfsFileSystemErrorSuccess)
					printf("Search: %s not found\n", name);
				else
					printf("Search: %s found\n", name);
				break;
			case 'd':
				printf("Delete: %s\n", name);
				if (tfs_delete_inode(fs, path) != TfsFileSystemErrorSuccess)
					printf("Delete: could not delete %s\n", name);
				else
					printf("Delete: %s successfully deleted\n", name);
				break;
			default: {
				/* error */
				fprintf(stderr, "Error: command to apply\n");
				continue;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	/* init filesystem */
	TfsFileSystem fs = tfs_new(100);

	/* process input and print tree */
	apply_commands(&fs);
	tfs_print(&fs, stdout);

	/* release allocated memory */
	tfs_drop(&fs);
	exit(EXIT_SUCCESS);
}
