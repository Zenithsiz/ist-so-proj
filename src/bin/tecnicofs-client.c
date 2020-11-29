/// @file
/// @brief Filesystem client
/// @details
/// This file serves as the client to the tfs.
/// @note
/// All static functions here, when encountering an error,
/// will simply report it and exit the program, as opposed
/// to returning an error.

#include <ctype.h>		// isspace
#include <errno.h>		// errno
#include <stdio.h>		// fprintf, stderr
#include <stdlib.h>		// size_t
#include <tfs/client.h> // tfs_client_*

/// @brief Processes all input from `in`, sending it to the server at `connection`
/// @param connection The server connection to send commands to
/// @param in Input file to read commands from
static void process_input(TfsClientServerConnection* connection, FILE* in);

/// @brief Opens the input file
/// @param in_filename Filename of the file to open in `in`. Or '-' for stdin.
/// @param[out] in Opened input file.
static void open_input(const char* in_filename, FILE** in);

/// @brief Closes the input file
/// @param in The input file.
static void close_input(FILE** in);

int main(int argc, char* argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: ./tecnicofs-client <input-file> <server-socket-name>\n");
		return EXIT_FAILURE;
	}

	// Open the input file
	FILE* in;
	open_input(argv[1], &in);

	// Start the client-server connection
	const char* server_path = argv[2];
	TfsClientServerConnectionNewResult connection_result = tfs_client_server_connection_new(server_path);
	if (!connection_result.success) {
		fprintf(stderr, "Unable to mount socket: %s\n", server_path);
		tfs_client_server_connection_new_error_print(&connection_result.data.err, stderr);
		return EXIT_FAILURE;
	}
	TfsClientServerConnection connection = connection_result.data.connection;
	printf("Mounted on the tfs server! (socket = %s)\n", server_path);

	// Process all input
	process_input(&connection, in);

	tfs_client_server_connection_destroy(&connection);
	close_input(&in);

	return EXIT_SUCCESS;
}

static void process_input(TfsClientServerConnection* connection, FILE* in) {
	for (size_t cur_line = 1;; cur_line++) {
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
		TfsCommandParseResult parse_result = tfs_command_parse(in);
		if (!parse_result.success) {
			fprintf(stderr, "Unable to parse line %zu\n", cur_line);
			tfs_command_parse_error_print(&parse_result.data.err, stderr);
			exit(EXIT_FAILURE);
		}

		// Then send it to the server
		TfsCommand command = parse_result.data.command;
		TfsClientServerConnectionSendCommandResult send_result =
			tfs_client_server_connection_send_command(connection, &command);
		tfs_command_destroy(&command);
		if (!send_result.success) {
			fprintf(stderr, "Unable to send command to server\n");
			fprintf(stderr, "(%d) %s\n", errno, strerror(errno));
			tfs_client_server_connection_send_command_error_print(&send_result.data.err, stderr);
			exit(EXIT_FAILURE);
		}

		if (!send_result.data.command_successful) {
			fprintf(stderr, "Failed to execute command in line %zu\n", cur_line);
		}
	}
}

static void open_input(const char* in_filename, FILE** in) {
	// Open the input file
	// Note: If we receive '-', use stdin
	if (strcmp(in_filename, "-") == 0) { *in = stdin; }
	else {
		*in = fopen(in_filename, "r");
		if (*in == NULL) {
			fprintf(stderr, "Unable to open input file '%s'\n", in_filename);
			fprintf(stderr, "(%d) %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

static void close_input(FILE** in) {
	// If `in` isn't stdin, close it
	if (*in != stdin) {
		int res = fclose(*in);
		if (res != 0) {
			fprintf(stderr, "Unable to close input file\n");
			fprintf(stderr, "(%d) %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// Then set it to `NULL`
	*in = NULL;
}
