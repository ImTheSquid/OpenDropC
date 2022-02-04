#include <stdlib.h>
#include <curl/curl.h>
#include <plist/plist.h>
#include <string.h>

#include "../include/client.h"
#include "config_private.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata);

struct opendrop_client_s {
    CURL *curl;
    char *latest_response;
    size_t latest_response_len;

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

    memset(*client, 0, sizeof(opendrop_client));

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
        curl_easy_setopt(curl_handle, CURLOPT_CAINFO_BLOB, config->root_ca) ||
        curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2) ||
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback) ||
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, *client)) {
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

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    opendrop_client *client = (opendrop_client*) userdata;

    size_t new_len = client->latest_response_len + size * nmemb;
    if (!(client->latest_response = realloc(client->latest_response, new_len + 1))) {
        return 0;
    }

    memcpy(client->latest_response + client->latest_response_len, ptr, size*nmemb);
    client->latest_response[new_len] = 0;
    return size * nmemb;
}

int opendrop_client_discover(opendrop_client *client, char **receiver_name) {
    int ret = 0;
    struct curl_slist *headers = generate_default_headers_list();
    headers = curl_slist_append(headers, "ContentType: application/octet-stream");

    if (curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers)) {
        curl_slist_free_all(headers);
        return 1;
    }

    plist_t root = plist_new_dict();

    if (client->config->record_data) {
        plist_dict_set_item(root, "SenderRecordData", client->config->record_data);
    }

    char *buf;
    uint32_t len;
    // Convert PLIST to binary format and null-terminate
    if (plist_to_bin(root, &buf, &len) || !(buf = realloc(buf, len + 1)) || curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, buf)) {
        ret = 1;
        goto DONE;
    }
    buf[len] = 0;

    free(client->latest_response);
    client->latest_response = NULL;
    client->latest_response_len = 0;
    if (curl_easy_perform(client->curl)) {
        ret = 1;
        goto DONE;
    }

    plist_t *response;
    plist_from_memory(client->latest_response, client->latest_response_len, response);

    plist_t receiver_comp = plist_dict_get_item(response, "ReceiverComputerName");
    if (receiver_comp) {
        char *receiver_comp_tmp;
        plist_get_string_val(receiver_comp, &receiver_comp_tmp);

        if (!(*receiver_name = (char*) realloc(*receiver_name, strlen(receiver_comp_tmp) + 1))) {
            plist_mem_free(receiver_comp_tmp);
            ret - 1;
            goto DONE;
        }

        strcpy(*receiver_name, receiver_comp_tmp);
        plist_mem_free(receiver_comp_tmp);
    }

DONE:
    curl_slist_free_all(headers);
    plist_free(root);
    if (receiver_comp) {
        plist_free(receiver_comp);
    }
    plist_free(response);
    return ret;
}

int opendrop_client_ask(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len, bool is_url, const opendrop_client_data *icon) {
    struct curl_slist *headers = generate_default_headers_list();
    headers = curl_slist_append(headers, "ContentType: application/octet-stream");



    curl_slist_free_all(headers);
    return 0;
}

int opendrop_client_send(const opendrop_client *client, const opendrop_client_data **data_arr, size_t data_arr_len) {
    struct curl_slist *headers = generate_default_headers_list();
    headers = curl_slist_append(headers, "ContentType: application/x-cpio");



    curl_slist_free_all(headers);
    return 0;
}