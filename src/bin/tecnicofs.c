/// @file
/// @brief Filesystem binary
/// @details
/// This file serves as the binary to interact with the
/// filesystem described in exercise 1 of the project.
/// @note
/// All static functions here, when encountering an error,
/// will simply report it and exit the program, as opposed
/// to returning an error.

#include <assert.h>			   // assert
#include <ctype.h>			   // isspace
#include <errno.h>			   // errno
#include <pthread.h>		   // pthread_create, pthread_join
#include <stddef.h>			   // size_t
#include <stdio.h>			   // fprintf, stderr, stdout, stdin
#include <string.h>			   // strerror
#include <sys/types.h>		   // ssize_t
#include <tfs/command/table.h> // TfsCommandTable
#include <tfs/fs.h>			   // TfsFs
#include <tfs/rw_lock.h>	   // TfsRwLock
#include <time.h>			   // timespec, clock_gettime

/// @brief Data received by each worker
typedef struct WorkerData {
	/// @brief File system
	TfsFs* fs;

	/// @brief Command table
	TfsCommandTable* command_table;
} WorkerData;

/// @brief Filesystem worker to run in each thread.
static void* worker_thread_fn(void* arg);

/// @brief Fills the command table from a file
static void fill_command_table(TfsCommandTable* table, FILE* in);

/// @brief Opens the input and output files
/// @param in_filename Filename of the file to open in `in`. Or '-' for stdin.
/// @param out_filename Filename of the file to open in `out`. Or '-' for stdout.
/// @param[out] in Opened input file.
/// @param[out] out Opened output file.
static void open_io(const char* in_filename, const char* out_filename, FILE** in, FILE** out);

/// @brief Closes the input and output files
/// @param in The input file.
/// @param out The output file.
static void close_io(FILE** in, FILE** out);

int main(int argc, char** argv) {
	if (argc != 4) {
		fprintf(stderr, "Usage: ./tecnicofs <input> <out> <num-threads>\n");
		return EXIT_FAILURE;
	}

	// Open the input and output files
	FILE* in;
	FILE* out;
	open_io(argv[1], argv[2], &in, &out);

	// Get number of threads
	char* argv_3_end;
	size_t num_threads = strtoul(argv[3], &argv_3_end, 0);
	if (argv_3_end == NULL || argv_3_end[0] != '\0' || (ssize_t)num_threads <= 0) {
		fprintf(stderr, "Unable to parse number of threads\n");
		return EXIT_FAILURE;
	}

	// Create the command table and the file system
	TfsCommandTable command_table = tfs_command_table_new(128);
	TfsFs fs = tfs_fs_new();

	// Bundle all data together for the workers
	WorkerData data = (WorkerData){
		.command_table = &command_table,
		.fs = &fs,
	};

	// Get the start time of the execution
	struct timespec start_time;
	assert(clock_gettime(CLOCK_REALTIME, &start_time) == 0);

	// Create all threads
	pthread_t worker_threads[num_threads];
	for (size_t n = 0; n < num_threads; n++) {
		int res = pthread_create(&worker_threads[n], NULL, worker_thread_fn, &data);
		if (res != 0) {
			fprintf(stderr, "Unable to create thread #%zu: %d\n", n, res);
			return EXIT_FAILURE;
		}
	}

	// Fill the command table
	fill_command_table(&command_table, in);

	// Then join them
	for (size_t n = 0; n < num_threads; n++) {
		int res = pthread_join(worker_threads[n], NULL);
		if (res != 0) {
			fprintf(stderr, "Unable to join thread #%zu: %d\n", n, res);
			return EXIT_FAILURE;
		}
	}

	// Print how long we took
	struct timespec end_time;
	assert(clock_gettime(CLOCK_REALTIME, &end_time) == 0);
	double diff_secs =
		(double)(end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_nsec - start_time.tv_nsec) / 10.0e9;
	fprintf(stdout, "TecnicoFS completed in %.4f seconds\n", diff_secs);

	// Print the tree before exiting
	tfs_fs_print(&fs, out);

	// Destroy all resources in reverse order of creation.
	tfs_command_table_destroy(&command_table);
	tfs_fs_destroy(&fs);
	close_io(&in, &out);

	return EXIT_SUCCESS;
}

static void* worker_thread_fn(void* arg) {
	WorkerData* data = arg;

	while (1) {
		// Pop a command from the table.
		// If it returns `false`, we should exit.
		TfsCommand command;
		if (!tfs_command_table_pop(data->command_table, &command)) { break; }

		// Execute each command on the file system
		// Note: We pass the command table lock to unlock once it's ready
		//       for more commands.
		// Note: On error we print the error backtrace and simply continue
		//       on to the next command
		switch (command.kind) {
			case TfsCommandCreate: {
				TfsInodeType inode_type = command.data.create.type;
				TfsPath path = tfs_path_owned_borrow(command.data.create.path);

				fprintf(stderr, "Creating %s '%.*s'\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);

				// Lock the filesystem and create the file
				TfsFsCreateResult result = tfs_fs_create(data->fs, path, inode_type);
				if (!result.success) {
					fprintf(stderr,
						"Unable to create %s '%.*s'\n",
						tfs_inode_type_str(inode_type),
						(int)path.len,
						path.chars);
					tfs_fs_create_error_print(&result.data.err, stderr);
				}
				else {
					TfsInodeIdx idx = result.data.idx;
					fprintf(stderr,
						"Successfully created %s '%.*s' (Inode %zu)\n",
						tfs_inode_type_str(inode_type),
						(int)path.len,
						path.chars,
						idx);
					tfs_fs_unlock_inode(data->fs, idx);
				}
				break;
			}

			// Delete path
			case TfsCommandRemove: {
				TfsPath path = tfs_path_owned_borrow(command.data.remove.path);

				fprintf(stderr, "Removing '%.*s'\n", (int)path.len, path.chars);

				TfsFsRemoveResult result = tfs_fs_remove(data->fs, path);
				if (!result.success) {
					fprintf(stderr, "Unable to remove '%.*s'\n", (int)path.len, path.chars);
					tfs_fs_remove_error_print(&result.data.err, stderr);
				}
				else {
					// Note: No need to unlock anything, as we just remove the inode
					fprintf(stderr, "Successfully removed '%.*s'\n", (int)path.len, path.chars);
				}
				break;
			}

			case TfsCommandSearch: {
				TfsPath path = tfs_path_owned_borrow(command.data.search.path);

				fprintf(stderr, "Searching '%.*s'\n", (int)path.len, path.chars);

				TfsFsFindResult result = tfs_fs_find(data->fs, path, TfsRwLockAccessShared);
				if (!result.success) {
					fprintf(stderr, "Unable to find '%.*s'\n", (int)path.len, path.chars);
					tfs_fs_find_error_print(&result.data.err, stderr);
				}
				else {
					TfsLockedInode inode = result.data.inode;
					fprintf(stderr,
						"Found %s '%.*s' (Inode %zu)\n",
						tfs_inode_type_str(inode.type),
						(int)path.len,
						path.chars,
						inode.idx);
					tfs_fs_unlock_inode(data->fs, inode.idx);
				}
				break;
			}

			case TfsCommandMove: {
				TfsPath source = tfs_path_owned_borrow(command.data.move.source);
				TfsPath dest = tfs_path_owned_borrow(command.data.move.dest);

				fprintf(stderr, "Moving '%.*s' to '%.*s'\n", (int)source.len, source.chars, (int)dest.len, dest.chars);

				TfsFsMoveResult result = tfs_fs_move(data->fs, source, dest, TfsRwLockAccessUnique);
				if (!result.success) {
					fprintf(stderr,
						"Unable to move '%.*s' to '%.*s'\n",
						(int)source.len,
						source.chars,
						(int)dest.len,
						dest.chars);
					tfs_fs_move_error_print(&result.data.err, stderr);
				}
				else {
					TfsLockedInode inode = result.data.inode;
					fprintf(stderr,
						"Successfully moved %s '%.*s' (Inode %zu) to '%.*s'\n",
						tfs_inode_type_str(inode.type),
						(int)source.len,
						source.chars,
						inode.idx,
						(int)dest.len,
						dest.chars);
					tfs_fs_unlock_inode(data->fs, inode.idx);
				}
				break;
			}

			default: {
			}
		}

		// Free the command
		tfs_command_destroy(&command);
	}

	return NULL;
}

static void fill_command_table(TfsCommandTable* table, FILE* in) {
	// Fill the comand table
	for (size_t cur_line = 0;; cur_line++) {
		// Skip any whitespace
		int last_char;
		while (last_char = fgetc(in), isspace(last_char)) {}
		ungetc(last_char, in);

		// If it starts with '#', skip this line
		int peek = fgetc(in);
		if (peek == '#') {
			while (fgetc(in) != '\n' && !feof(in)) {}
			continue;
		}
		else {
			ungetc(peek, in);
		}

		// If the line is empty, stop
		if (feof(in)) { break; }

		// Try to parse it
		TfsCommandParseError parse_err;
		TfsCommand command;
		if (!tfs_command_parse(in, &command, &parse_err)) {
			fprintf(stderr, "Unable to parse line %zu\n", cur_line);
			tfs_command_parse_error_print(&parse_err, stderr);
			exit(EXIT_FAILURE);
		}

		// Then push it
		tfs_command_table_push(table, command);
	}

	tfs_command_table_writer_exit(table);
}

static void open_io(const char* in_filename, const char* out_filename, FILE** in, FILE** out) {
	// Open the input file
	// Note: If we receive '-', use stdin
	if (strcmp(in_filename, "-") == 0) { *in = stdin; }
	else {
		*in = fopen(in_filename, "r");
		if (*in == NULL) {
			fprintf(stderr, "Unable to open input file '%s'\n", in_filename);
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// Open the output file
	// Note: If we receive '-', use stdout
	if (strcmp(out_filename, "-") == 0) { *out = stdout; }
	else {
		*out = fopen(out_filename, "w");
		if (*out == NULL) {
			fprintf(stderr, "Unable to open output file '%s'\n", out_filename);
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

static void close_io(FILE** in, FILE** out) {
	// If `in` isn't stdin, close it
	if (*in != stdin) {
		int res = fclose(*in);
		if (res != 0) {
			fprintf(stderr, "Unable to close input file\n");
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// If `out` isn't stdout, close it
	if (*out != stdout) {
		int res = fclose(*out);
		if (res != 0) {
			fprintf(stderr, "Unable to close output file\n");
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// Then set both to `NULL`
	*in = NULL;
	*out = NULL;
}
