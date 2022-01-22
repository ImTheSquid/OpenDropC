#include <stdio.h>
#include <string.h>

#include "../include/browser.h"
#include "../include/config.h"

int test_browser();
int test_server();

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Not enough arguments supplied");
        return 1;
    }

    if (!strcmp(argv[1], "browser")) {
        return test_browser();
    } else if (!strcmp(argv[1], "server")) {
        return test_server();
    } else if (!strcmp(argv[1], "config")) {
        return test_config();
    }

    return 2;
}

/*
BROWSER TESTING
*/

void browser_add_service(opendrop_browser* b, const opendrop_service* s, void* userdata) {

}

void browser_remove_service(opendrop_browser* b, const char*  name, const char* type, const char* domain, void* userdata) {

}

void browser_status(opendrop_browser* b, opendrop_browser_status s, void* userdata) {

}

int test_browser() {
    opendrop_browser *browser = NULL;

    if (opendrop_browser_new(&browser, "lo")) {
        printf("CREATE ERROR %i: %s", opendrop_browser_init_errno(), opendrop_browser_strerror(opendrop_browser_init_errno()));
        return 1;
    }

    opendrop_browser_set_state_callback(browser, browser_status, NULL);
    opendrop_browser_set_add_service_callback(browser, browser_add_service, NULL);
    opendrop_browser_set_remove_service_callback(browser, browser_remove_service, NULL);

    if (opendrop_browser_start(browser)) {
        printf("START ERROR %i: %s", opendrop_browser_errno(browser), opendrop_browser_strerror(opendrop_browser_errno(browser)));
        return 1;
    }

    opendrop_browser_stop(browser);

    opendrop_browser_free(browser);

    return 0;
}

/*
SERVER TESTING
*/

int test_server() {
    return 0;
}

/*
CONFIG TESTING
*/

int test_config() {
    opendrop_config *config;
    unsigned char array[13] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64, 0x21};
    if (opendrop_config_new(&config, array, 13)) {
        printf("CREATE ERROR %i: %s", opendrop_config_init_errno(), opendrop_config_strerror(opendrop_config_init_errno()));
        return 1;
    }

    opendrop_config_free(config);

    return 0;
}