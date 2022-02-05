#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "../include/browser.h"
#include "../include/client.h"
#include "config_private.h"
#include "../include/server.h"

enum Action {
    ACTION_RECEIVE,
    ACTION_FIND,
    ACTION_SEND
};

struct arguments {
    enum Action action;
    opendrop_config *config;
};

static char docs[] = "\
    This utility expects the Apple root certificate as certs/apple_root_ca.pem\
";

static char args_doc[] = "<receive|find|send>";

error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = (struct arguments*) state->input;

    switch (key) {
        case 'i':
            args->config->interface = arg;
            break;

        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;

        case ARGP_KEY_ARG:
            if (!strcmp(arg, "receive")) {
                args->action = ACTION_RECEIVE;
            } else if (!strcmp(arg, "find")) {
                args->action = ACTION_FIND;
            } else if (!strcmp(arg, "send")) {
                args->action = ACTION_SEND;
            } else {
                argp_usage(state);
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static struct argp_option options[] = {
    { "file", 'f', "FILE", OPTION_ARG_OPTIONAL, "File to be sent" },
    { "url", 'u', "URL", OPTION_ARG_OPTIONAL, "-f, --file is a URL" },
    { "receiver", 'r', "RECEIVER", OPTION_ARG_OPTIONAL, "Peer to send file to (can be index, ID, or hostname)" },
    { "name", 'n', "NAME", OPTION_ARG_OPTIONAL, "Computer name (displayed in sharing pane)" },
    { "model", 'm', "MODEL", OPTION_ARG_OPTIONAL, "Computer model (displayed in sharing pane)" },
    { "interface", 'i', "INTERFACE", OPTION_ARG_OPTIONAL, "Which AWDL interface to use, defaults to awdl0" },
    { 0 }
};

static struct argp argp = { options, parse_opt, args_doc, docs };

sigset_t mask, oldmask;
opendrop_browser * browser;

void int_handler(int val) {
    if (browser) {
        opendrop_browser_free(browser);
        browser = NULL;
    }
}

void browser_add_service(opendrop_browser* b, const opendrop_service* s, void* userdata) {
    printf("Service Added:\nNAME: %s\nHOST NAME: %s\nADDRESS: %s\nPORT: %u\nTYPE: %s\nDOMAIN: %s\n", s->name, s->host_name, s->address, s->port, s->type, s->domain);
}

void browser_remove_service(opendrop_browser* b, const char* name, const char* type, const char* domain, void* userdata) {
    printf("Service Removed:\nNAME: %s\nTYPE: %s\nDOMAIN: %s\n", name, type, domain);
}

void browser_status(opendrop_browser* b, opendrop_browser_status s, void* userdata) {
    printf("Status Change: ");
    switch (s) {
        case OPENDROP_BROWSER_CACHE_EMPTY:
            printf("Cache empty\n");
            break;
        case OPENDROP_BROWSER_DONE:
            printf("All done, there probably won't be anything new for a while, stopping...\n");
            opendrop_browser_free(browser);
            browser = NULL;
            break;
        case OPENDROP_BROWSER_ERROR:
            printf("Error: %s\n", opendrop_browser_strerror(opendrop_browser_errno(browser)));
            opendrop_browser_free(browser);
            browser = NULL;
            break;
    }
}

int find(const opendrop_config *config) {
    printf("Creating browser bound to interface \"%s\"\n", config->interface);

    int err;
    if (err = opendrop_browser_new(&browser, config->interface)) {
        printf("Failed to create browser: %s\n", opendrop_browser_strerror(opendrop_browser_init_errno(err)));
        return 1;
    }

    opendrop_browser_set_state_callback(browser, browser_status, NULL);
    opendrop_browser_set_add_service_callback(browser, browser_add_service, NULL);
    opendrop_browser_set_remove_service_callback(browser, browser_remove_service, NULL);

    if (opendrop_browser_start(browser)) {
        printf("Failed to start browser: %s\n", opendrop_browser_strerror(opendrop_browser_errno(browser)));
        return 1;
    }

    printf("Ctrl+C to stop browser\n");

    // Wait for signal
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while (browser) {
        sigsuspend(&oldmask);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    printf("\nSIGINT! Shutdown successful\n");

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Required args: <receive|find|send>\n");
        return 1;
    }

    struct arguments args;

    if (access("certs/apple_root_ca.pem", R_OK)) {
        printf("certs/apple_root_ca.pem not found\n");
        return 1;
    }

    printf("Loading root certificate...\n");

    FILE *root_cert = fopen("certs/apple_root_ca.pem", "r");
    fseek(root_cert, 0, SEEK_END);
    size_t file_size = ftell(root_cert);
    fseek(root_cert, 0, SEEK_SET);
    unsigned char *cert_data = (unsigned char*) malloc(file_size);
    if (!cert_data) {
        fclose(root_cert);
        printf("Failed to alloc cert_data\n");
        return 1;
    }

    fread(cert_data, sizeof(unsigned char), file_size, root_cert);
    fclose(root_cert);

    if (opendrop_config_new(&args.config, cert_data, file_size)) {
        printf("Failed to create config\n");
        return 1;
    }
    printf("Root certificate loaded\n");

    argp_parse(&argp, argc, argv, 0, 0, &args);

    // Set up signals and pausing
    struct sigaction act = {0};
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);

    // Figure out what to do
    switch (args.action) {
        case ACTION_FIND:
            return find(args.config);
    }
    
    return 1;
}