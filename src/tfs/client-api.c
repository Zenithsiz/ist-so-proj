#include "client-api.h"

// Imports
#include <assert.h> // assert
#include <stdlib.h> // exit, EXIT_FAILURE
#include <unistd.h> // getpid, unlink, close, open

void tfs_client_server_connection_new_error_print(const TfsClientServerConnectionNewError* self, FILE* out) {
	switch (self->kind) {
		case TfsClientServerConnectionNewErrorCreateSocket: {
			fprintf(out, "Unable to create socket\n");
			break;
		}
		case TfsClientServerConnectionNewErrorBindSocket: {
			fprintf(out, "Unable to bind socket\n");
			break;
		}
		default: {
			break;
		}
	}
}

void tfs_client_server_connection_send_command_error_print( //
	const TfsClientServerConnectionSendCommandError* self,
	FILE* out //
) {
	switch (self->kind) {
		case TfsClientServerConnectionSendCommandErrorSend: {
			fprintf(out, "Unable to send command\n");
			break;
		}
		case TfsClientServerConnectionSendCommandErrorReceive: {
			fprintf(out, "Unable to receive response\n");
			break;
		}
		default: {
			break;
		}
	}
}

TfsClientServerConnectionNewResult tfs_client_server_connection_new(const char* server_path) {
	// Create our socket
	int client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (client_socket < 0) {
		return (TfsClientServerConnectionNewResult){
			.success = false,
			.data.err.kind = TfsClientServerConnectionNewErrorCreateSocket,
		};
	}

	// Create the client address and get it's length
	struct sockaddr_un client_address;
	pid_t pid = getpid();
	bzero(&client_address, sizeof(struct sockaddr_un));
	client_address.sun_family = AF_UNIX;
	snprintf(client_address.sun_path, 108, "/tmp/tfs-client-%d", pid);
	socklen_t client_address_len = (socklen_t)SUN_LEN(&client_address);

	// Unlink and bind our socket
	unlink(client_address.sun_path);
	int bind_res = bind(client_socket, (struct sockaddr*)&client_address, client_address_len);
	if (bind_res < 0) {
		return (TfsClientServerConnectionNewResult){
			.success = false,
			.data.err.kind = TfsClientServerConnectionNewErrorBindSocket,
		};
	}

	// Create the server address and get it's length
	struct sockaddr_un server_address;
	bzero(&server_address, sizeof(struct sockaddr_un));
	server_address.sun_family = AF_UNIX;
	strcpy(server_address.sun_path, server_path);
	socklen_t server_address_len = (socklen_t)SUN_LEN(&server_address);

	return (TfsClientServerConnectionNewResult){
		.success = true,
		.data.connection.client_socket = client_socket,
		.data.connection.client_address = client_address,
		.data.connection.client_address_len = client_address_len,
		.data.connection.server_address = server_address,
		.data.connection.server_address_len = server_address_len,
	};
}

void tfs_client_server_connection_destroy(TfsClientServerConnection* connection) {
	/// Close the socket
	close(connection->client_socket);

	/// And unlink our socket
	unlink(connection->client_address.sun_path);
}

TfsClientServerConnectionSendCommandResult tfs_client_server_connection_send_command(TfsClientServerConnection* self,
	const TfsCommand* command //
) {
	// Build the command string
	char command_str[1024];
	tfs_command_to_string(command, command_str, sizeof(command_str));
	size_t command_str_len = strlen(command_str);

	// Send the string to the server
	ssize_t characters_sent = sendto(self->client_socket,
		command_str,
		command_str_len + 1,
		0,
		(struct sockaddr*)&self->server_address,
		self->server_address_len //
	);
	if (characters_sent < 0) {
		return (TfsClientServerConnectionSendCommandResult){
			.success = false,
			.data.err.kind = TfsClientServerConnectionSendCommandErrorSend,
		};
	}
	assert((size_t)characters_sent == command_str_len + 1);

	// Receive the response from the server.
	// Note: Response is either '\x00' for failure, or '\x01' for success.
	char response_buffer;
	ssize_t characters_received = recv(self->client_socket, &response_buffer, 1, 0);
	if (characters_received < 0) {
		return (TfsClientServerConnectionSendCommandResult){
			.success = false,
			.data.err.kind = TfsClientServerConnectionSendCommandErrorReceive,
		};
	}

	return (TfsClientServerConnectionSendCommandResult){
		.success = true,
		.data.command_successful = response_buffer != '\0',
	};
}

/// @brief Global client connection for the API.
static TfsClientServerConnection global_client_connection;

/// @brief If the global client is currently initialized
static bool global_client_connection_initialized = false;

int tfsCreate(char* path, char type) {
	TfsInodeType new_type;
	switch (type) {
		case 'f': {
			new_type = TfsInodeTypeFile;
			break;
		}
		case 'd': {
			new_type = TfsInodeTypeDir;
			break;
		}
		default: {
			return 1;
		}
	}

	TfsPathOwned new_path = tfs_path_to_owned(tfs_path_from_cstr(path));

	TfsCommand command =
		(TfsCommand){.kind = TfsCommandCreate, .data.create.path = new_path, .data.create.type = new_type};
	TfsClientServerConnectionSendCommandResult result =
		tfs_client_server_connection_send_command(&global_client_connection, &command);
	tfs_command_destroy(&command);
	if (!result.success) { return 1; }

	if (!result.data.command_successful) { return 2; }

	return 0;
}

int tfsDelete(char* path) {
	TfsPathOwned new_path = tfs_path_to_owned(tfs_path_from_cstr(path));

	TfsCommand command = (TfsCommand){.kind = TfsCommandRemove, .data.remove.path = new_path};
	TfsClientServerConnectionSendCommandResult result =
		tfs_client_server_connection_send_command(&global_client_connection, &command);
	tfs_command_destroy(&command);
	if (!result.success) { return 1; }

	if (!result.data.command_successful) { return 2; }

	return 0;
}

int tfsLookup(char* path) {
	TfsPathOwned new_path = tfs_path_to_owned(tfs_path_from_cstr(path));

	TfsCommand command = (TfsCommand){.kind = TfsCommandSearch, .data.search.path = new_path};
	TfsClientServerConnectionSendCommandResult result =
		tfs_client_server_connection_send_command(&global_client_connection, &command);
	tfs_command_destroy(&command);
	if (!result.success) { return 1; }

	if (!result.data.command_successful) { return 2; }

	return 0;
}

int tfsMove(char* from, char* to) {
	TfsPathOwned new_from = tfs_path_to_owned(tfs_path_from_cstr(from));
	TfsPathOwned new_to = tfs_path_to_owned(tfs_path_from_cstr(to));

	TfsCommand command = (TfsCommand){.kind = TfsCommandMove, .data.move.source = new_from, .data.move.dest = new_to};
	TfsClientServerConnectionSendCommandResult result =
		tfs_client_server_connection_send_command(&global_client_connection, &command);
	tfs_command_destroy(&command);
	if (!result.success) { return 1; }

	if (!result.data.command_successful) { return 2; }

	return 0;
}

int tfsPrint(char* path) {
	char* new_path = strdup(path);

	TfsCommand command = (TfsCommand){.kind = TfsCommandPrint, .data.print.path = new_path};
	TfsClientServerConnectionSendCommandResult result =
		tfs_client_server_connection_send_command(&global_client_connection, &command);
	tfs_command_destroy(&command);
	if (!result.success) { return 1; }

	if (!result.data.command_successful) { return 2; }

	return 0;
}

int tfsMount(char* server_path) {
	if (global_client_connection_initialized) {
		tfs_client_server_connection_destroy(&global_client_connection);
		global_client_connection_initialized = false;
	}

	TfsClientServerConnectionNewResult result = tfs_client_server_connection_new(server_path);
	if (!result.success) {
		fprintf(stderr, "Unable to initialize the client connections\n");
		tfs_client_server_connection_new_error_print(&result.data.err, stderr);
		return 1;
	}
	global_client_connection = result.data.connection;

	return 0;
}

int tfsUnmount(void) {
	if (!global_client_connection_initialized) {
		fprintf(stderr, "Tried to unmount while no connection was alive");
		return 1;
	}

	tfs_client_server_connection_destroy(&global_client_connection);
	global_client_connection_initialized = false;

	return 0;
}
