#include <memory.h>
#include <unistd.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include "config_private.h"
#include "../include/config.h"

// Recovered from sharingd`receiverSupportsX methods.
// A valid node needs to either have SUPPORTS_PIPELINING or SUPPORTS_MIXED_TYPES
// according to sharingd`[SDBonjourBrowser removeInvalidNodes:].
// Default flags on macOS: 0x3fb according to sharingd`[SDRapportBrowser defaultSFNodeFlags]
#define OPENDROP_AIRDROP_SUPPORTS_URL 0x01
#define OPENDROP_AIRDROP_SUPPORTS_DVZIP 0x02
#define OPENDROP_AIRDROP_SUPPORTS_PIPELINING 0x04
#define OPENDROP_AIRDROP_SUPPORTS_MIXED_TYPES 0x08
#define OPENDROP_AIRDROP_SUPPORTS_UNKNOWN1 0x10
#define OPENDROP_AIRDROP_SUPPORTS_UNKNOWN2 0x20
#define OPENDROP_AIRDROP_SUPPORTS_IRIS 0x40
#define OPENDROP_AIRDROP_SUPPORTS_DISCOVER_MAYBE 0x80
#define OPENDROP_AIRDROP_SUPPORTS_UNKNOWN3 0x100
#define OPENDROP_AIRDROP_SUPPORTS_ASSET_BUNDLE 0x200

int last_config_init_error = 0;
bool srand_called = false;

int opendrop_config_new(opendrop_config **config, const unsigned char *root_ca, size_t root_ca_len) {
    if (!(root_ca && root_ca_len)) {
        last_config_init_error = 1;
        return 1;
    }

    opendrop_config *config_unwrap = *config;

    if (!(config_unwrap = (opendrop_config*) malloc(sizeof(opendrop_config)))) {
        last_config_init_error = 2;
        return 1;
    }

    if (!(config_unwrap->root_ca = (struct curl_blob*) malloc(sizeof(struct curl_blob)))) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 2;
        return 1;
    }

    if (!(config_unwrap->root_ca->data = malloc(root_ca_len))) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 2;
        return 1;
    }

    memcpy(config_unwrap->root_ca->data, root_ca, root_ca_len);
    config_unwrap->root_ca->len = root_ca_len;
    config_unwrap->root_ca->flags = CURL_BLOB_NOCOPY;

    // Initialize other default config values
    if (gethostname(config_unwrap->host_name, HOST_NAME_MAX)) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 3;
        return 1;
    }

    // Make sure string is null-terminated
    config_unwrap->host_name[HOST_NAME_MAX] = '\0';

    config_unwrap->computer_model = (char*) malloc(strlen("OpenDrop") + 1);
    strcpy(config_unwrap->computer_model, "OpenDrop");
    config_unwrap->computer_name = (char*) malloc(HOST_NAME_MAX);
    strcpy(config_unwrap->computer_name, config_unwrap->host_name);

    config_unwrap->server_port = 8771;

    if (!srand_called) {
        srand(time(NULL));
        srand_called = true;
    }
    
    sprintf(config_unwrap->service_id, "%.6x", rand());

    config_unwrap->interface = (char*) malloc(strlen("awdl0") + 1);
    strcpy(config_unwrap->interface, "awdl0");

    config_unwrap->email = NULL;
    config_unwrap->phone = NULL;

    config_unwrap->flags = OPENDROP_AIRDROP_SUPPORTS_MIXED_TYPES | OPENDROP_AIRDROP_SUPPORTS_DISCOVER_MAYBE;

    config_unwrap->record_data = NULL;

    // Create default certificate and key
    EVP_PKEY *pkey;
    if (!(pkey = EVP_PKEY_new())) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 4;
        return 1;
    }

    RSA *rsa;
    if (RSA_generate_key_ex(rsa, 2048, NULL, NULL)) {
        EVP_PKEY_free(pkey);
        opendrop_config_free(config_unwrap);
        last_config_init_error = 4;
        return 1;
    }

    EVP_PKEY_assign_RSA(pkey, rsa);

    X509 *x509;
    if (!(x509 = X509_new())) {
        RSA_free(rsa);
        EVP_PKEY_free(pkey);
        opendrop_config_free(config_unwrap);
        last_config_init_error = 4;
        return 1;
    }

    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
    X509_set_pubkey(x509, pkey);

    X509_NAME * name;
    name = X509_get_subject_name(x509);

    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC,
                            (unsigned char *)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC,
                            (unsigned char *)"OpenDrop Inc.", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                            (unsigned char *)"localhost", -1, -1, 0);

    X509_set_issuer_name(x509, name);
    X509_sign(x509, pkey, EVP_sha3_512());

    // Write certs to temp files then read them back into memory
    FILE *key_file = tmpfile();
    PEM_write_PrivateKey(key_file, pkey, EVP_des_ede3_cbc(), "openDropKey", 11, NULL, NULL);
    fflush(key_file);
    FILE *cert_file = tmpfile();
    PEM_write_X509(cert_file, x509);
    fflush(cert_file);

    X509_free(x509);
    RSA_free(rsa);
    EVP_PKEY_free(pkey);

    long key_file_size = ftell(key_file);
    rewind(key_file);

    config_unwrap->key_data->len = key_file_size;
    config_unwrap->key_data->flags = CURL_BLOB_NOCOPY;
    config_unwrap->key_data->data = malloc(key_file_size);
    if (fread(config_unwrap->key_data->data, 1, key_file_size, key_file) != key_file_size) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 4;
        return 1;
    }

    fclose(key_file);

    long cert_file_size = ftell(cert_file);
    rewind(cert_file);

    config_unwrap->cert_data->len = cert_file_size;
    config_unwrap->cert_data->flags = CURL_BLOB_NOCOPY;
    config_unwrap->cert_data->data = malloc(cert_file_size);
    if (fread(config_unwrap->cert_data->data, 1, cert_file_size, cert_file) != cert_file_size) {
        opendrop_config_free(config_unwrap);
        last_config_init_error = 4;
        return 1;
    }

    return 0;
}

void opendrop_config_free(opendrop_config *config) {
    if (config) {
        free(config->computer_name);
        free(config->computer_model);
        free(config->interface);

        if (config->root_ca) {
            free(config->root_ca->data);
            free(config->root_ca);
        }

        if (config->cert_data) {
            free(config->cert_data->data);
            free(config->cert_data);
        }

        if (config->key_data) {
            free(config->key_data->data);
            free(config->key_data);
        }

        free(config);
    }
}

void opendrop_config_set_host_name(opendrop_config *config, const char *host_name) {
    strncpy(config->host_name, host_name, HOST_NAME_MAX + 1);
    config->host_name[HOST_NAME_MAX] = '\0';
}

int opendrop_config_set_computer_name(opendrop_config *config, const char *computer_name) {
    if (!(config->computer_name = (char*) realloc(config->computer_name, sizeof(computer_name) + 1))) {
        return 1;
    }

    strcpy(config->computer_name, computer_name);
    return 0;
}

int opendrop_config_set_computer_model(opendrop_config *config, const char *computer_model) {
    if (!(config->computer_model = (char*) realloc(config->computer_model, strlen(computer_model) + 1))) {
        return 1;
    }

    strcpy(config->computer_model, computer_model);
    return 0;
}

void opendrop_config_set_server_port(opendrop_config *config, uint16_t port) {
    config->server_port = port;
}

void opendrop_config_set_service_id(opendrop_config *config, const char *service_id) {
    strncpy(config->service_id, service_id, 6);
    config->service_id[6] = '\0';
}

int opendrop_config_set_interface(opendrop_config *config, const char *interface) {
    if (!(config->interface = (char*) realloc(config->interface, strlen(interface) + 1))) {
        return 1;
    }

    strcpy(config->interface, interface);
    return 0;
}

int opendrop_config_set_email(opendrop_config *config, const char *email) {
    if (!(config->email = (char*) realloc(config->email, strlen(email) + 1))) {
        return 1;
    }

    strcpy(config->email, email);
    return 0;
}

int opendrop_config_set_phone(opendrop_config *config, const char *phone) {
    if (!(config->phone = (char*) realloc(config->phone, strlen(phone) + 1))) {
        return 1;
    }

    strcpy(config->phone, phone);
    return 0;
}

int opendrop_config_set_cert(opendrop_config *config, const unsigned char *cert_data, size_t cert_data_len, const unsigned char *key_data, size_t key_data_len) {
    if (!(config->cert_data->data = realloc(config->cert_data->data, cert_data_len))) {
        return 1;
    }

    memcpy(config->cert_data->data, cert_data, cert_data_len);

    if (!(config->key_data->data = realloc(config->key_data->data, key_data_len))) {
        return 1;
    }

    memcpy(config->key_data->data, key_data, key_data_len);

    return 0;
}

int opendrop_config_set_record_data(opendrop_config *config, void *record_data, size_t record_data_len) {
    if (!config->record_data) {
        if (!(config->record_data = (struct curl_blob*) malloc(sizeof(struct curl_blob)))) {
            return 1;
        }

        config->record_data->flags = CURL_BLOB_NOCOPY;
    }

    if (!(config->record_data->data = realloc(config->record_data->data, record_data_len))) {
        return 1;
    }

    config->record_data->len = record_data_len;
    memcpy(config->record_data->data, record_data, record_data_len);

    return 0;
}

int opendrop_config_init_errno() {
    return last_config_init_error;
}

const char *opendrop_config_strerror(int code) {
    switch (code) {
    case 1: return "No root CA or no root CA length given.";
    case 2: return "Failed to allocate memory.";
    case 3: return "Failed to get host name.";
    case 4: return "Failed to generate default certificate.";
    }

    return "Unknown error.";
}