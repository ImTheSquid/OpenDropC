#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct opendrop_config_s opendrop_config;

// Initializes OpenDrop config instance with default values
// - config: OpenDrop config
// - root_ca: Apple root CA data, copied to internal buffer
// - root_ca_len: Length of CA data
int opendrop_config_new(opendrop_config **config, const unsigned char *root_ca, size_t root_ca_len);

void opendrop_config_free(opendrop_config *config);

// Setter functions
// Returns 0 on success, 1 if malloc failed
void opendrop_config_set_host_name(opendrop_config *config, const char *host_name);

int opendrop_config_set_computer_name(opendrop_config *config, const char *computer_name);

int opendrop_config_set_computer_model(opendrop_config *config, const char *computer_model);

void opendrop_config_set_server_port(opendrop_config *config, uint16_t port);

void opendrop_config_set_service_id(opendrop_config *config, const char *service_id);

int opendrop_config_set_interface(opendrop_config *config, const char *interface);

int opendrop_config_set_email(opendrop_config *config, const char *email);

int opendrop_config_set_phone(opendrop_config *config, const char *phone);

int opendrop_config_set_cert(opendrop_config *config, const unsigned char *cert_data, size_t cert_data_len, const unsigned char *key_data, size_t key_data_len);

int opendrop_config_set_record_data(opendrop_config *config, void *record_data, size_t record_data_len);

// Error functions
int opendrop_config_init_errno();

const char *opendrop_config_strerror(int code);