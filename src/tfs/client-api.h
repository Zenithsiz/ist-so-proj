/// @file
/// @brief Client API
/// @details
/// API to be used by the clients to send requests to the server

#ifndef TFS_CLIENT_API_H
#define TFS_CLIENT_API_H

// Imports
#include <tfs/error.h> // TfsError

TfsError tfsCreate(char* path, char nodeType);
TfsError tfsDelete(char* path);
TfsError tfsLookup(char* path);
TfsError tfsMove(char* from, char* to);
TfsError tfsMount(char* serverName);
TfsError tfsUnmount();

#endif
