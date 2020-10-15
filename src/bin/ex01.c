/// @file
/// @brief Exercice 1 binary
/// @details
/// This file implements exercice 1 of the project
/// using the tfs library.
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
#include <tfs/command/table.h> // TfsCommandTable
#include <tfs/fs.h>			   // TfsFs
#include <tfs/lock.h>		   // TfsLock
#include <time.h>			   // timespec, clock_gettime

/// @brief Data received by each worker
typedef struct WorkerData {
	/// @brief File system
	TfsFs* fs;

	/// @brief Command table
	TfsCommandTable* command_table;

	/// @brief Command lock
	/// @details
	/// This lock serves to ensure commands are executed
	/// as-if sequentially. The lock is locked for writes
	/// when popping an element from the table. After this
	/// the file system locks the relevant inodes for the
	/// execution of the command and unlocks the command
	/// table lock, allowing other commands to be executed.
	TfsLock* command_table_lock;

	/// @brief File system lock
	/// @details
	/// This lock is used to synchronize the file system
	/// globally, for the first exercice.
	TfsLock* fs_lock;
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

// TODO: Check why this _sometimes_ segfaults in malloc and in other places due to memory corruption.
int main(int argc, char** argv) {
	if (argc != 5) {
		fprintf(stderr, "Usage: ./ex01 <input> <out> <num-threads> <sync>\n");
		return EXIT_FAILURE;
	}

	// Open the input and output files
	FILE* in;
	FILE* out;
	open_io(argv[1], argv[2], &in, &out);

	// Get number of threads
	char* argv_3_end;
	size_t num_threads = strtoul(argv[3], &argv_3_end, 0);
	if (argv_3_end == NULL || argv_3_end[0] != '\0') {
		fprintf(stderr, "Unable to parse number of threads");
		return EXIT_FAILURE;
	}

	// Get sync strategy
	TfsLockKind lock_kind;
	if (strcmp(argv[4], "mutex") == 0) {
		lock_kind = TfsLockKindMutex;
	}
	else if (strcmp(argv[4], "rwlock") == 0) {
		lock_kind = TfsLockKindRWLock;
	}
	else if (strcmp(argv[4], "nosync") == 0) {
		lock_kind = TfsLockKindNone;
	}
	else {
		fprintf(stderr, "Invalid sync strategy '%s'", argv[4]);
		return EXIT_FAILURE;
	}

	// Create the command table and it's lock, then fill it from the input file.
	// Note: The command table lock is always a mutex, regardless of
	//       the sync strategy.
	TfsCommandTable* command_table = tfs_command_table_new();
	TfsLock command_table_lock	   = tfs_lock_new(TfsLockKindMutex);
	fill_command_table(command_table, in);

	// Create the file system and it's lock
	TfsFs fs		= tfs_fs_new(lock_kind);
	TfsLock fs_lock = tfs_lock_new(lock_kind);

	// Bundle all data together for the workers
	WorkerData data = (WorkerData){
		.command_table		= command_table,
		.fs					= &fs,
		.command_table_lock = &command_table_lock,
		.fs_lock			= &fs_lock,
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
	double diff_secs = (double)(end_time.tv_sec - start_time.tv_sec) + (double)(end_time.tv_nsec - start_time.tv_nsec) / 10.0e9;
	fprintf(out, "TecnicoFS completed in %.4f seconds", diff_secs);

	// Print the tree before exiting
	tfs_fs_print(&fs, out);

	// Destroy all resources in reverse order of creation.
	tfs_lock_destroy(&fs_lock);
	tfs_lock_destroy(&command_table_lock);
	tfs_command_table_destroy(command_table);
	tfs_fs_destroy(&fs);
	close_io(&in, &out);

	return EXIT_SUCCESS;
}

static void* worker_thread_fn(void* arg) {
	WorkerData* data = arg;

	while (1) {
		// Lock and pop a command from the table
		tfs_lock_lock(data->command_table_lock, TfsLockAccessUnique);
		TfsCommand command;
		if (!tfs_command_table_pop(data->command_table, &command)) {
			tfs_lock_unlock(data->command_table_lock);
			break;
		}

		// Execute each command on the file system
		// Note: We pass the command table lock to unlock once it's ready
		//       for more commands.
		// Note: On error we print the error backtrace and simply continue
		//       on to the next command
		switch (command.kind) {
			case TfsCommandCreate: {
				TfsInodeType inode_type = command.data.create.type;
				TfsPath path			= tfs_path_from_owned(command.data.create.path);

				fprintf(stderr, "Creating %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);

				// Lock the filesystem and create the file
				TfsFsCreateError err;
				tfs_lock_lock(data->fs_lock, TfsLockAccessUnique);
				TfsInodeIdx idx = tfs_fs_create(data->fs, path, inode_type, data->command_table_lock, &err);
				tfs_lock_unlock(data->fs_lock);
				if (idx == TFS_INODE_IDX_NONE) {
					fprintf(stderr, "Unable to create %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
					tfs_fs_create_error_print(&err, stderr);
				}
				else {
					fprintf(stderr, "Successfully created %s %.*s (Inode %zu)\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars, idx);
					// SAFETY: We received it locked from `tfs_fs_create`
					assert(tfs_fs_unlock_inode(data->fs, idx));
				}
				break;
			}

			// Delete path
			case TfsCommandRemove: {
				TfsPath path = tfs_path_from_owned(command.data.remove.path);

				fprintf(stderr, "Removing %.*s\n", (int)path.len, path.chars);

				TfsFsRemoveError err;
				tfs_lock_lock(data->fs_lock, TfsLockAccessUnique);
				if (!tfs_fs_remove(data->fs, path, data->command_table_lock, &err)) {
					fprintf(stderr, "Unable to remove %.*s\n", (int)path.len, path.chars);
					tfs_fs_remove_error_print(&err, stderr);
				}
				else {
					// Note: No need to unlock anything, as we just remove the inode
					fprintf(stderr, "Successfully removed %.*s\n", (int)path.len, path.chars);
				}
				tfs_lock_unlock(data->fs_lock);
				break;
			}

			case TfsCommandSearch: {
				TfsPath path = tfs_path_from_owned(command.data.search.path);

				fprintf(stderr, "Searching %.*s\n", (int)path.len, path.chars);

				TfsInodeType inode_type;
				TfsFsFindError err;
				tfs_lock_lock(data->fs_lock, TfsLockAccessShared);
				TfsInodeIdx idx = tfs_fs_find(data->fs, path, data->command_table_lock, TfsLockAccessShared, &inode_type, NULL, &err);
				tfs_lock_unlock(data->fs_lock);
				if (idx == TFS_INODE_IDX_NONE) {
					fprintf(stderr, "Unable to find %.*s\n", (int)path.len, path.chars);
					tfs_fs_find_error_print(&err, stderr);
				}
				else {
					fprintf(stderr, "Found %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
					// SAFETY: We received it locked from `tfs_fs_find`.
					assert(tfs_fs_unlock_inode(data->fs, idx));
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
		if (feof(in)) {
			break;
		}

		// Try to parse it
		TfsCommandParseError parse_err;
		TfsCommand command;
		if (!tfs_command_parse(in, &command, &parse_err)) {
			fprintf(stderr, "Unable to parse line %zu\n", cur_line);
			tfs_command_parse_error_print(&parse_err, stderr);
			exit(EXIT_FAILURE);
		}

		// Then push it
		if (!tfs_command_table_push(table, command)) {
			fprintf(stderr, "Unable to push command onto table");
			exit(EXIT_FAILURE);
		}
	}
}

static void open_io(const char* in_filename, const char* out_filename, FILE** in, FILE** out) {
	// Open the input file
	// Note: If we receive '-', use stdin
	if (strcmp(in_filename, "-") == 0) {
		*in = stdin;
	}
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
	if (strcmp(out_filename, "-") == 0) {
		*out = stdout;
	}
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
		fclose(*in);
	}

	// If `out` isn't stdout, close it
	if (*out != stdout) {
		fclose(*out);
	}

	// Then set both to `NULL`
	*in	 = NULL;
	*out = NULL;
}
