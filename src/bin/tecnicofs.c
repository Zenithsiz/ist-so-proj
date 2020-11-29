/// @file
/// @brief Filesystem server
/// @details
/// This file serves as the server to the tfs.
/// @note
/// All static functions here, when encountering an error,
/// will simply report it and exit the program, as opposed
/// to returning an error.

#include <assert.h>				 // assert
#include <ctype.h>				 // isspace
#include <errno.h>				 // errno
#include <pthread.h>			 // pthread_create, pthread_join
#include <stddef.h>				 // size_t
#include <stdio.h>				 // fprintf, stderr, stdout, stdin
#include <stdlib.h>				 // EXIT_FAILURE,
#include <string.h>				 // strerror
#include <sys/socket.h>			 // socket, bind
#include <sys/types.h>			 // ssize_t, AF_UNIX, SOCK_DGRAM
#include <sys/un.h>				 // sockaddr_un
#include <tfs/command/command.h> // TfsCommand
#include <tfs/fs.h>				 // TfsFs
#include <tfs/rw_lock.h>		 // TfsRwLock
#include <time.h>				 // timespec, clock_gettime
#include <unistd.h>				 // unlink

/// @brief Data received by each worker
typedef struct WorkerData {
	/// @brief File system
	TfsFs* fs;

	/// @brief Our socket
	int server_socket;
} WorkerData;

/// @brief Filesystem worker to run in each thread.
static void* worker_thread_fn(void* arg);

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: ./tecnicofs <num-threads> <socket-name>\n");
		return EXIT_FAILURE;
	}

	// Get number of threads
	char* num_threads_end;
	size_t num_threads = strtoul(argv[1], &num_threads_end, 0);
	if (num_threads_end == NULL || num_threads_end[0] != '\0' || (ssize_t)num_threads <= 0) {
		fprintf(stderr, "Unable to parse number of threads\n");
		return EXIT_FAILURE;
	}

	// Create the file system
	TfsFs fs = tfs_fs_new();

	// Create the server socket
	int server_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (server_socket < 0) {
		fprintf(stderr, "Unable to create server socket\n");
		fprintf(stderr, "(%d) %s\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}

	// Create the socket address for ourselves
	const char* server_socket_path = argv[2];
	struct sockaddr_un server_address;
	bzero(&server_address, sizeof(struct sockaddr_un));
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, server_socket_path);

	// Unlink and bind our socket
	socklen_t server_address_len = (socklen_t)SUN_LEN(&server_address);
	unlink(server_socket_path);
	int bind_res = bind(server_socket, (struct sockaddr*)&server_address, server_address_len);
	if (bind_res < 0) {
		fprintf(stderr, "Unable to bind server socket\n");
		fprintf(stderr, "(%d) %s\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}

	// Bundle up the worker data
	WorkerData data = (WorkerData){
		.fs = &fs,
		.server_socket = server_socket,
	};

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

	// Destroy all resources in reverse order of creation.
	close(server_socket);
	unlink(argv[1]);
	tfs_fs_destroy(&fs);

	return EXIT_SUCCESS;
}

static void* worker_thread_fn(void* arg) {
	WorkerData* data = arg;

	while (1) {
		// Receive the command
		char command_str[512];
		struct sockaddr_un client_address;
		socklen_t client_address_len = sizeof(client_address);
		ssize_t command_str_len = recvfrom(data->server_socket,
			command_str,
			sizeof(command_str) - 1,
			0,
			(struct sockaddr*)&client_address,
			&client_address_len //
		);
		if (command_str_len <= 0) {
			fprintf(stderr, "Failed to receive command\n");
			exit(EXIT_FAILURE);
		}

		// Parse the command string
		command_str[command_str_len] = '\0';
		FILE* command_input = fmemopen(command_str, (size_t)command_str_len, "r");
		TfsCommandParseResult parse_result = tfs_command_parse(command_input);
		fclose(command_input);
		if (!parse_result.success) {
			// Respond with negative
			char response = '\0';
			sendto(data->server_socket, &response, 1, 0, (struct sockaddr*)&client_address, client_address_len);

			fprintf(stderr, "Unable to parse command: \"%s\"\n", command_str);
			tfs_command_parse_error_print(&parse_result.data.err, stderr);
			continue;
		}
		TfsCommand command = parse_result.data.command;

		// Execute each command on the file system
		// Note: On error we print the error backtrace and simply continue
		//       on to the next command
		bool executed_successfully;
		switch (command.kind) {
			case TfsCommandCreate: {
				TfsInodeType inode_type = command.data.create.type;
				TfsPath path = tfs_path_owned_borrow(command.data.create.path);

				fprintf(stderr, "Creating %s '%.*s'\n", tfs_inode_type_str(inode_type), (int)path.len, path.chars);

				// Lock the filesystem and create the file
				TfsFsCreateResult result = tfs_fs_create(data->fs, path, inode_type);
				executed_successfully = result.success;
				if (!executed_successfully) {
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
						idx.idx);
					tfs_fs_unlock_inode(data->fs, idx);
				}
				break;
			}

			// Delete path
			case TfsCommandRemove: {
				TfsPath path = tfs_path_owned_borrow(command.data.remove.path);

				fprintf(stderr, "Removing '%.*s'\n", (int)path.len, path.chars);

				TfsFsRemoveResult result = tfs_fs_remove(data->fs, path);
				executed_successfully = result.success;
				if (!executed_successfully) {
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
				executed_successfully = result.success;
				if (!executed_successfully) {
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
						inode.idx.idx);
					tfs_fs_unlock_inode(data->fs, inode.idx);
				}
				break;
			}

			case TfsCommandMove: {
				TfsPath source = tfs_path_owned_borrow(command.data.move.source);
				TfsPath dest = tfs_path_owned_borrow(command.data.move.dest);

				fprintf(stderr, "Moving '%.*s' to '%.*s'\n", (int)source.len, source.chars, (int)dest.len, dest.chars);

				TfsFsMoveResult result = tfs_fs_move(data->fs, source, dest, TfsRwLockAccessUnique);
				executed_successfully = result.success;
				if (!executed_successfully) {
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
						inode.idx.idx,
						(int)dest.len,
						dest.chars);
					tfs_fs_unlock_inode(data->fs, inode.idx);
				}
				break;
			}

			case TfsCommandPrint: {
				const char* file_name = command.data.print.path;

				fprintf(stderr, "Printing filesystem to '%s'\n", file_name);

				TfsFsPrintResult result = tfs_fs_print(data->fs, file_name);
				executed_successfully = result.success;
				if (!executed_successfully) {
					fprintf(stderr, "Unable to print filesystem to '%s'\n", file_name);
					tfs_fs_print_error_print(&result.data.err, stderr);
				}
				else {
					fprintf(stderr, "Successfully printed filesystem to '%s'\n", file_name);
				}
				break;
			}

			default: {
			}
		}

		// Free the command
		tfs_command_destroy(&command);

		// Respond if we were successful
		char response = executed_successfully ? '\1' : '\0';
		sendto(data->server_socket, &response, 1, 0, (struct sockaddr*)&client_address, client_address_len);
	}

	return NULL;
}
