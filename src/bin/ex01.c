#include <ctype.h>
#include <errno.h> // errno
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			   // strerror
#include <tfs/command/table.h> // TfsCommandTable
#include <tfs/fs.h>

int main(int argc, char** argv) {
	if (argc != 5) {
		fprintf(stderr, "Usage: ./ex01 <input> <out> <num-threads> <sync>");
		return EXIT_FAILURE;
	}

	// Open the input file
	// Note: If we receive '-', use stdin
	FILE* in;
	if (strcmp(argv[1], "-") == 0) {
		in = stdin;
	}
	else {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			fprintf(stderr, "Unable to open input file '%s': %s\n", argv[1]);
			fprintf(stderr, "%s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	// Open the output file
	// Note: If we receive '-', use stdout
	FILE* out;
	if (strcmp(argv[2], "-") == 0) {
		out = stdout;
	}
	else {
		out = fopen(argv[2], "r");
		if (out == NULL) {
			fprintf(stderr, "Unable to open output file '%s'\n", argv[2]);
			fprintf(stderr, "%s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	// Create the file system
	TfsFs fs = tfs_fs_new();

	// Create the command table
	TfsCommandTable* table = tfs_command_table_new(TfsCommandTableLockKindMutex);

	// Fill the comand table
	for (size_t cur_line = 0;; cur_line++) {
		// If it starts with '#', skip this line
		int peek = fgetc(in);
		if (peek == '#') {
			while (fgetc(in) != '\n' && !feof(in)) {}
		}
		else {
			ungetc(peek, in);
		}

		// Try to parse it
		TfsCommandParseResult parse_res = tfs_command_parse(in);
		if (parse_res.kind != TfsCommandParseResultSuccess) {
			fprintf(stderr, "Unable to parse line %zu\n", cur_line);
			tfs_command_parse_result_print(&parse_res, stderr);
			return EXIT_FAILURE;
		}

		// Then push it
		printf("Adding new command (%zu)\n", cur_line);
		TfsCommandTablePushResult push_res = tfs_command_table_push(table, parse_res.data.success.command);
		if (push_res != TfsCommandTablePushResultSuccess) {
			fprintf(stderr, "Unable to push command onto table");
			tfs_command_table_push_result_print(&push_res, stderr);
			return EXIT_FAILURE;
		}

		// If the line is empty, stop
		if (feof(in)) {
			break;
		}
	}

	// TODO: Execute all commands

	// Print the tree before exiting
	tfs_fs_print(&fs, out);

	// And destroy the file system.
	tfs_fs_destroy(&fs);

	return EXIT_SUCCESS;
}
