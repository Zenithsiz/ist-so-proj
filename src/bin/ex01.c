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
} WorkerData;

/// @brief Worker function
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

		switch (command.kind) {
			case TfsCommandCreate: {
				TfsInodeType inode_type = command.data.create.type;
				TfsPath path			= tfs_path_from_owned(command.data.create.path);

				fprintf(stderr, "Creating %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
				TfsFsCreateError err;
				TfsInodeIdx idx = tfs_fs_create(data->fs, path, inode_type, data->command_table_lock, &err);
				if (idx == TFS_INODE_IDX_NONE) {
					fprintf(stderr, "Unable to create %s %.*s\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);
					tfs_fs_create_error_print(&err, stderr);
				}
				else {
					fprintf(stderr, "Successfully created %s %.*s (Inode %zu)\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars, idx);
				}

				break;
			}

			case TfsCommandSearch: {
				TfsPath path = tfs_path_from_owned(command.data.search.path);

				TfsFsFindError err;
				TfsInodeIdx idx = tfs_fs_find(data->fs, path, data->command_table_lock, TfsLockAccessShared, &err);
				if (idx == TFS_INODE_IDX_NONE) {
					fprintf(stderr, "Unable to find %.*s\n", (int)path.len, path.chars);
					tfs_fs_find_error_print(&err, stderr);
				}
				else {
					fprintf(stderr, "Found %.*s\n", (int)path.len, path.chars);
					tfs_fs_unlock_inode(data->fs, idx);
				}
				break;
			}

			// Delete path
			case TfsCommandRemove: {
				TfsPath path = tfs_path_from_owned(command.data.remove.path);

				fprintf(stderr, "Deleting %.*s\n", (int)path.len, path.chars);
				TfsFsRemoveError err;
				if (!tfs_fs_remove(data->fs, path, data->command_table_lock, &err)) {
					fprintf(stderr, "Unable to delete %.*s\n", (int)path.len, path.chars);
					tfs_fs_remove_error_print(&err, stderr);
				}
				else {
					fprintf(stderr, "Successfully deleted %.*s\n", (int)path.len, path.chars);
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

int main(int argc, char** argv) {
	if (argc != 5) {
		fprintf(stderr, "Usage: ./ex01 <input> <out> <num-threads> <sync>\n");
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
			fprintf(stderr, "Unable to open input file '%s'\n", argv[1]);
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

	// Get the start time of the execution
	struct timespec start_time;
	assert(clock_gettime(CLOCK_REALTIME, &start_time) == 0);

	// Create the file system
	TfsFs fs = tfs_fs_new(lock_kind);

	// Create the command table
	TfsCommandTable* command_table = tfs_command_table_new();

	// Create the command table lock
	// Note: The command table lock is always a mutex, regardless of
	//       the sync strategy.
	TfsLock command_table_lock = tfs_lock_new(TfsLockKindMutex);

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
			return EXIT_FAILURE;
		}

		// Then push it
		if (!tfs_command_table_push(command_table, command)) {
			fprintf(stderr, "Unable to push command onto table");
			return EXIT_FAILURE;
		}
	}

	// Bundle all data together for the workers
	WorkerData data = (WorkerData){
		.command_table		= command_table,
		.fs					= &fs,
		.command_table_lock = &command_table_lock,
	};

	// VLA with all worker threads
	pthread_t worker_threads[num_threads];

	// Create all threads
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

	// Destroy the lock
	tfs_lock_destroy(&command_table_lock);

	// Destroy the command table
	tfs_command_table_destroy(command_table);

	// And destroy the file system.
	tfs_fs_destroy(&fs);

	// If `in` isn't stdin, close it
	if (in != stdin) {
		fclose(in);
	}

	// If `out` isn't stdout, close it
	if (out != stdout) {
		fclose(out);
	}

	return EXIT_SUCCESS;
}
