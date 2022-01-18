#pragma once

#include <curl/curl.h>
#include <curl/easy.h>
#include <stdint.h>

struct opendrop_config_s {
    char host_name[HOST_NAME_MAX + 1];
    char *computer_name;
    char *computer_model;
    
    uint16_t server_port;

    char service_id[7]; // 6 byte string + term

    char *interface;

    char *email;
    char *phone;

    uint8_t flags;

    // Certs
    struct curl_blob *root_ca;
    struct curl_blob *cert_data;
    struct curl_blob *key_data;

    struct curl_blob *record_data;
};