/// @file
/// @brief Client API
/// @details
/// API to be used by the clients to send requests to the server

#ifndef TFS_CLIENT_API_H
#define TFS_CLIENT_API_H

// Imports
#include <stdio.h>				 // FILE
#include <sys/socket.h>			 // socklen_t
#include <sys/types.h>			 // <Compatibility>
#include <sys/un.h>				 // sockaddr_un
#include <tfs/command/command.h> // TfsCommand

/// @brief A server connection
typedef struct TfsClientServerConnection {
	/// @brief Our socket address
	struct sockaddr_un client_address;

	/// @brief The client's address length
	socklen_t client_address_len;

	/// @brief The server's address
	struct sockaddr_un server_address;

	/// @brief The server's address length
	socklen_t server_address_len;

	/// @brief Our socket
	int client_socket;
} TfsClientServerConnection;

/// @brief Error type for #tfs_client_server_connection_new
typedef struct TfsClientServerConnectionNewError {
	/// @brief Error kind
	enum {
		/// @brief Unable to create the client socket
		TfsClientServerConnectionNewErrorCreateSocket,

		/// @brief Unable to bind client socket to path
		TfsClientServerConnectionNewErrorBindSocket,
	} kind;
} TfsClientServerConnectionNewError;

/// @brief Result type for #tfs_client_server_connection_new
typedef struct TfsClientServerConnectionNewResult {
	/// @brief If successful
	bool success;

	/// @brief Result data
	union {
		/// @brief Success connection
		TfsClientServerConnection connection;

		/// @brief Underlying error
		TfsClientServerConnectionNewError err;
	} data;
} TfsClientServerConnectionNewResult;

/// @brief Error type for #tfs_client_server_connection_new
typedef struct TfsClientServerConnectionSendCommandError {
	/// @brief Error kind
	enum {
		/// @brief Unable to send command to server
		TfsClientServerConnectionSendCommandErrorSend,

		/// @brief Unable to receive message from server
		TfsClientServerConnectionSendCommandErrorReceive,
	} kind;
} TfsClientServerConnectionSendCommandError;

/// @brief Result type for #tfs_client_server_connection_send_command
typedef struct TfsClientServerConnectionSendCommandResult {
	/// @brief If successful
	bool success;

	/// @brief Result data
	union {
		/// @brief If the command from the server was successful
		bool command_successful;

		/// @brief Underlying error
		TfsClientServerConnectionSendCommandError err;
	} data;
} TfsClientServerConnectionSendCommandResult;

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_client_server_connection_new_error_print(const TfsClientServerConnectionNewError* self, FILE* out);

/// @brief Prints a textual representation of @p self to @p out
/// @param self
/// @param out File to output to.
void tfs_client_server_connection_send_command_error_print( //
	const TfsClientServerConnectionSendCommandError* self,
	FILE* out //
);

/// @brief Creates a new connection to the server
/// @param server_path The path of the socket to connect to
TfsClientServerConnectionNewResult tfs_client_server_connection_new(const char* server_path);

/// @brief Destroys a connection to the server
void tfs_client_server_connection_destroy(TfsClientServerConnection* connection);

/// @brief Sends a message to the tfs server
/// @param self
/// @param command The command to send
TfsClientServerConnectionSendCommandResult tfs_client_server_connection_send_command( //
	TfsClientServerConnection* self,
	const TfsCommand* command //
);

/// @brief Sends a create command to the tfs server on the global client connection
/// @param path Path to create
/// @param type Type of inode to create
/// @return `0` on success
int tfsCreate(const char* path, char type);

/// @brief Sends a remove command to the tfs server on the global client connection
/// @param path Path to remove
/// @return `0` on success
int tfsDelete(const char* path);

/// @brief Sends a search command to the tfs server on the global client connection
/// @param path Path to search
/// @return `0` on success
int tfsLookup(const char* path);

/// @brief Sends a move command to the tfs server on the global client connection
/// @param source Source path to move
/// @param dest Destination path to move
/// @return `0` on success
int tfsMove(const char* from, const char* to);

/// @brief Sends a print command to the tfs server on the global client connection
/// @param source Output path to print to
/// @return `0` on success
int tfsPrint(const char* path);

/// @brief Mounts the global client connection with a server on `server_path`
/// @param server_path Path of the server to mount on.
/// @return `0` on success
int tfsMount(const char* server_path);

/// @brief Unmounts server on the global client connection
/// @return `0` on success
int tfsUnmount(void);

#endif
