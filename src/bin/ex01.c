#include <ctype.h>
#include <errno.h> // errno
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			   // strerror
#include <tfs/command/table.h> // TfsCommandTable
#include <tfs/fs.h>

/// @brief Data received by each worker
typedef struct WorkerData {
	/// @brief File system
	TfsFs* fs;

	/// @brief Command table
	TfsCommandTable* command_table;
} WorkerData;

/// @brief Worker function
static void* worker_thread_fn(void* arg) {
	WorkerData* data = arg;

	while (1) {
		TfsCommandTablePopResult pop_res = tfs_command_table_pop(data->command_table);
		if (!pop_res.is_some) {
			break;
		}
		TfsCommand command = pop_res.data.command;

		switch (command.kind) {
			// Create path
			case TfsCommandCreate: {
				TfsInodeType inode_type = command.data.create.type;
				TfsPath path			= tfs_path_from_owned(command.data.create.path);

				fprintf(stderr, "Creating %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
				TfsFsCreateResult res = tfs_fs_create(data->fs, inode_type, path);
				if (res.kind != TfsFsCreateResultSuccess) {
					fprintf(stderr, "Unable to create %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
					tfs_fs_create_result_print(&res, stderr);
				}
				else {
					fprintf(stderr, "Successfully created %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
				}

				break;
			}

			// Look up path
			case TfsCommandSearch: {
				TfsPath path = tfs_path_from_owned(command.data.search.path);

				TfsFsFindResult res = tfs_fs_find(data->fs, path);
				if (res.kind != TfsFsFindResultSuccess) {
					fprintf(stderr, "Unable to find %.*s\n", (int)path.len, path.chars);
					tfs_fs_find_result_print(&res, stderr);
				}
				else {
					fprintf(stderr, "Found %.*s\n", (int)path.len, path.chars);
				}
				break;
			}

			// Delete path
			case TfsCommandRemove: {
				TfsPath path = tfs_path_from_owned(command.data.remove.path);

				fprintf(stderr, "Deleting %.*s\n", (int)path.len, path.chars);
				TfsFsRemoveResult res = tfs_fs_remove(data->fs, path);
				if (res.kind != TfsFsRemoveResultSuccess) {
					fprintf(stderr, "Unable to delete %.*s\n", (int)path.len, path.chars);
					tfs_fs_remove_result_print(&res, stderr);
				}
				else {
					fprintf(stderr, "Successfully deleted %.*s (with index %zu)\n", (int)path.len, path.chars, res.data.success.idx);
				}
				break;
			}
			default: {
			}
		}
	}

	return NULL;
}

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

	// Get number of threads
	char* argv_3_end;
	size_t num_threads = strtoul(argv[3], &argv_3_end, 0);
	if (argv_3_end == NULL || argv_3_end[0] != '\0') {
		fprintf(stderr, "Unable to parse number of threads");
		return EXIT_FAILURE;
	}

	// Create the file system
	TfsFs fs = tfs_fs_new();

	// Create the command table
	TfsCommandTable* command_table = tfs_command_table_new(TfsKindMutex);

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
		TfsCommandTablePushResult push_res = tfs_command_table_push(command_table, parse_res.data.success.command);
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

	// Bundle all data together for the workers
	WorkerData data = (WorkerData){
		.command_table = command_table,
		.fs			   = &fs,
	};

	// VLA with all worker threads
	pthread_t worker_threads[num_threads];

	// Create all threads
	for (size_t n = 0; n < num_threads; n++) {
		printf("Creating new thread (%zu)\n", n);
		int res = pthread_create(&worker_threads[n], NULL, worker_thread_fn, &data);
		if (res != 0) {
			fprintf(stderr, "Unable to create thread #%zu: %d\n", n, res);
			return EXIT_FAILURE;
		}
	}

	// Then join them
	for (size_t n = 0; n < num_threads; n++) {
		printf("Joining thread (%zu)\n", n);
		int res = pthread_join(worker_threads[n], NULL);
		if (res != 0) {
			fprintf(stderr, "Unable to join thread #%zu: %d\n", n, res);
			return EXIT_FAILURE;
		}
	}

	// Print the tree before exiting
	tfs_fs_print(&fs, out);

	// And destroy the file system.
	tfs_fs_destroy(&fs);

	return EXIT_SUCCESS;
}
