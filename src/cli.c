#include <argp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "../include/browser.h"
#include "../include/client.h"
#include "../include/config.h"
#include "../include/server.h"

enum Action {
    ACTION_RECEIVE,
    ACTION_FIND,
    ACTION_SEND
};

struct arguments {
    enum Action action;
};

static char args_doc[] = "<receive|find|send>";

error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = (struct arguments*) state->input;

    switch (key) {
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

static struct argp argp = { options, parse_opt, args_doc, 0 };

void int_handler(int val) {
    
}

int main(int argc, char **argv) {
    struct arguments args;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    struct sigaction act;
    act.sa_handler = int_handler;
    sigaction(SIGINT, &act, NULL);
    
    return 0;
}