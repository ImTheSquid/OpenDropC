#include <stdlib.h>
#include <curl/curl.h>
#include <libxml/xmlwriter.h>

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

#define curl_handle (*client)->curl
    // Set regular values
    if (curl_easy_setopt(curl_handle, CURLOPT_INTERFACE, config->interface) || 
        curl_easy_setopt(curl_handle, CURLOPT_PORT, target_port) || 
        curl_easy_setopt(curl_handle, CURLOPT_URL, target_address) ||
        curl_easy_setopt(curl_handle, CURLOPT_SSLCERT_BLOB, config->cert_data) ||
        curl_easy_setopt(curl_handle, CURLOPT_CAINFO_BLOB, config->root_ca)) {
        opendrop_client_free(*client);
        last_client_init_error = -3;
        return 1;
    }
#undef curl_handle

    return 0;
}

struct curl_slist *generate_default_headers_list() {
    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "Connection: keep-alive");
    list = curl_slist_append(list, "Accept: */*");
    list = curl_slist_append(list, "User-Agent: AirDrop/1.0");
    list = curl_slist_append(list, "Accept-Language: en-us");
    list = curl_slist_append(list, "Accept-Encoding: br, gzip, deflate");
    list = curl_slist_append(list, "ContentType: application/octet-stream");

    return list;
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

int opendrop_client_ask(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len, bool is_url, const opendrop_client_data *icon) {
    
}

int opendrop_client_send(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len) {

}