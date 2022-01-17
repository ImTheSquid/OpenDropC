#include <stdlib.h>
#include <curl/curl.h>

#include "../include/client.h"
#include "config_private.h"

struct opendrop_client_s {
    CURL *curl;

    const opendrop_config *config;
};

size_t curl_refs = 0;

int last_client_init_error = 0;

int opendrop_client_new(opendrop_client **client, const char *target_address, uint16_t target_port, const opendrop_config *config) {
    // Create global cURL context
    if (!curl_refs++) {
        if (last_client_init_error = curl_global_init(CURL_GLOBAL_ALL)) {
            return 1;
        }
    }

    if (!(*client = (opendrop_client*) malloc(sizeof(opendrop_client)))) {
        last_client_init_error = -1;
        return 1;
    }

    if(!((*client)->curl = curl_easy_init())) {
        opendrop_client_free(*client);
        last_client_init_error = -2;
        return 1;
    }

    (*client)->config = config;

    return 0;
}

void opendrop_client_free(opendrop_client *client) {
    if (client) {
        if (client->curl) {
            curl_easy_cleanup(client->curl);
        }

        free(client);

        if (!curl_refs || !(--curl_refs)) {
            curl_global_cleanup();
        }
    }
}