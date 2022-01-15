#include "config.h"

typedef struct opendrop_server_s opendrop_server;

// Initializes OpenDrop server
// Args:
// - server: OpenDrop server
// - config: OpenDrop config
// Return: 0 on success, >0 on error
int opendrop_server_new(opendrop_server *server, const opendrop_config *config);

// Frees OpenDrop server
// Args:
// - server: OpenDrop server
void opendrop_server_free(opendrop_server *server);

// Starts OpenDrop server
// Args:
// - server: OpenDrop server
// Returns:  0 on success, >0 on error
int opendrop_server_start(opendrop_server *server);

// Stops OpenDrop server
// Args:
// - server: OpenDrop server
void opendrop_server_stop(opendrop_server *server);