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

// Sends ASK request to server to see if ready to accept
// Args:
// - client: OpenDrop client
// - data_arr: An array of pointers to client data that will be sent
// - data_arr_len: Number of datas to be sent
// - is_url: If set to true, only the first item in data_arr will be sent (as a URL)
// - icon: Optional icon to send to AirDrop system, must be image in JPEG2000 form
// - icon_len: Size of icon
// Returns: 0 on success, >0 on error
int opendrop_client_ask(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len, bool is_url, const unsigned char *icon, size_t icon_len);

// Attempts to send file
// Args:
// - client: OpenDrop client
// - data_arr: An array of pointers to client data that will be sent
// - data_arr_len: Number of datas to be sent
// Returns: 0 on success, >0 on error
int opendrop_client_send(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len);