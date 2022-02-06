#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef struct opendrop_client_s opendrop_client;

typedef struct opendrop_client_data_s {
    unsigned char *data;
    size_t data_len;
} opendrop_client_data;

typedef struct opendrop_client_file_data_s {
    char *name;
    char *type;
    char *bom_path;
    bool is_dir;

    // These values may be NULL/0 for ASK requests
    unsigned char *data;
    size_t data_len;
} opendrop_client_file_data;

// Initializes OpenDrop client
// Args:
// - client: OpenDrop client
// - target_address: The address to attempt to connect to
// - target_port: The port to attempt to connect to
// - config: OpenDrop config instance
// Returns: 0 on success, >0 on error
int opendrop_client_new(opendrop_client **client, const char *target_address, uint16_t target_port, const opendrop_config *config);

// Frees OpenDrop client
// Args:
// - client: OpenDrop client
void opendrop_client_free(opendrop_client *client);

// Sends DISCOVER request to server to show record data
// Args:
// - client: OpenDrop client
// - receiver_name: Receiver name, allocated and populated if discoverable, not automatically freed
// Returns: 0 on success, >0 on error
int opendrop_client_discover(opendrop_client *client, char **receiver_name);

// Sends ASK request to server to see if ready to accept file or URL
// Args:
// - client: OpenDrop client
// - data_arr: An array of pointers to client data that will be sent
// - data_arr_len: Number of datas to be sent
// - is_url: If set to true, only the first item in data_arr will be sent (as a URL)
// - icon: Optional icon to send to AirDrop system, must be image in JPEG2000 form
// Returns: 0 on success, >0 on error
int opendrop_client_ask(opendrop_client *client, const opendrop_client_file_data **data_arr, size_t data_arr_len, bool is_url, const opendrop_client_data *icon);

// Attempts to send file, DO NOT USE TO SEND A URL
// Args:
// - client: OpenDrop client
// - data_arr: An array of pointers to client data that will be sent
// - data_arr_len: Number of datas to be sent
// Returns: 0 on success, >0 on error
int opendrop_client_send(const opendrop_client *client, const opendrop_client_file_data **data_arr, size_t data_arr_len);