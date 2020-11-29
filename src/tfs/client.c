#include "client.h"

// Imports
#include <assert.h> // assert
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

#include <errno.h>

TfsClientServerConnectionSendCommandResult tfs_client_server_connection_send_command(
	TfsClientServerConnection* connection,
	const TfsCommand* command //
) {
	// Build the command string
	char command_str[1024];
	tfs_command_to_string(command, command_str, sizeof(command_str));
	size_t command_str_len = strlen(command_str);

	// Send the string to the server
	ssize_t characters_sent = sendto(connection->client_socket,
		command_str,
		command_str_len + 1,
		0,
		(struct sockaddr*)&connection->server_address,
		connection->server_address_len //
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
	ssize_t characters_received = recv(connection->client_socket, &response_buffer, 1, 0);
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
