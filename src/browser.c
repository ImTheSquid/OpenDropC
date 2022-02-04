#include <stdlib.h>
#include <net/if.h>
#include <string.h>
#include <avahi-common/error.h>
#include <avahi-common/thread-watch.h>
#include <avahi-client/lookup.h>
#include "../include/browser.h"

#include <stdio.h>

struct opendrop_browser_s {
    AvahiClient *avahi_client;
    unsigned int interface_idx;
    AvahiServiceBrowser *avahi_browser;

    opendrop_browser_status_cb browser_status;
    void *status_userdata;

    opendrop_browser_service_add_cb service_add;
    void *add_userdata;

    opendrop_browser_service_remove_cb service_remove;
    void *remove_userdata;

    int last_avahi_error;
};

// Main loop with reference count
AvahiThreadedPoll *avahi_loop = NULL;
size_t watch_refs = 0;

int last_browser_init_error = 0;

// Handles Avahi client events
void client_callback(AvahiClient *client, AvahiClientState state, void *userdata) {
    opendrop_browser *browser = (opendrop_browser*) userdata;

    if (state == AVAHI_CLIENT_FAILURE) {
        browser->last_avahi_error = avahi_client_errno(client);
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->status_userdata);
    }
}

int opendrop_browser_new(opendrop_browser **browser, const char *interface) {
    // Create Avahi main loop
    if (!watch_refs++) {
        avahi_loop = avahi_threaded_poll_new();
        if (!avahi_loop || avahi_threaded_poll_start(avahi_loop)) {
            last_browser_init_error = 1;
            return 1;
        }
    }

    // Allocate browser struct
    if (!(*browser = (opendrop_browser*) malloc(sizeof(opendrop_browser)))) {
        last_browser_init_error = 2;
        return 1;
    }

    memset(*browser, 0, sizeof(opendrop_browser));

    // Find interface index
    if (!((*browser)->interface_idx = if_nametoindex(interface))) {
        opendrop_browser_free(*browser);
        last_browser_init_error = 3;
        return 1;
    }

    // Create Avahi client
    avahi_threaded_poll_lock(avahi_loop);
    int err;
    if (!((*browser)->avahi_client = avahi_client_new(avahi_threaded_poll_get(avahi_loop), 0, client_callback, *browser, &err))) {
        avahi_threaded_poll_unlock(avahi_loop);
        opendrop_browser_free(*browser);
        last_browser_init_error = err;
        return 1;
    }
    avahi_threaded_poll_unlock(avahi_loop);

    return 0;
}

void opendrop_browser_free(opendrop_browser *browser) {
    if (browser) {
        if (browser->avahi_browser) {
            avahi_service_browser_free(browser->avahi_browser);
        }

        if (browser->avahi_client) {
            avahi_threaded_poll_lock(avahi_loop);
            avahi_client_free(browser->avahi_client);
            avahi_threaded_poll_unlock(avahi_loop);
        }
        
        free(browser);

        // Avahi loop dealloc
        if (!watch_refs || !(--watch_refs)) {
            avahi_threaded_poll_free(avahi_loop);
        }
    }
}

// Handles Avahi resolver events
void resolve_callback(
    AvahiServiceResolver *r, 
    AvahiIfIndex interface, 
    AvahiProtocol protocol, 
    AvahiResolverEvent event, 
    const char *name, 
    const char *type, 
    const char *domain, 
    const char *host_name, 
    const AvahiAddress *address, 
    uint16_t port, 
    AvahiStringList *txt, 
    AvahiLookupResultFlags flags, 
    void *userdata) {
    
    opendrop_browser *browser = (opendrop_browser*) userdata;

    switch (event) {
    case AVAHI_RESOLVER_FAILURE:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->status_userdata);
        break;

    case AVAHI_RESOLVER_FOUND: ;
        opendrop_service service;
        memcpy(service.address, address->data.ipv6.address, 16);
        service.name = name;
        service.type = type;
        service.domain = domain;
        service.host_name = host_name;
        service.port = port;

        (*browser->service_add)(browser, &service, browser->add_userdata);
    }

    avahi_service_resolver_free(r);
}

// Handles Avahi browser events
void browse_callback(
    AvahiServiceBrowser *b, 
    AvahiIfIndex interface, 
    AvahiProtocol protocol, 
    AvahiBrowserEvent event, 
    const char *name, 
    const char *type, 
    const char *domain, 
    AvahiLookupResultFlags flags, 
    void *userdata) {
    
    opendrop_browser *browser = (opendrop_browser*) userdata;

    switch (event) {
    case AVAHI_BROWSER_NEW:
        if (!avahi_service_resolver_new(browser->avahi_client, interface, protocol, name, type, domain, AVAHI_PROTO_INET6, 0, resolve_callback, browser)) {
            (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->status_userdata);
        }
        break;

    case AVAHI_BROWSER_REMOVE:
        (*browser->service_remove)(browser, name, type, domain, browser->remove_userdata);
        break;

    case AVAHI_BROWSER_FAILURE:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->status_userdata);
        break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_DONE, browser->status_userdata);
        break;

    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_CACHE_EMPTY, browser->status_userdata);
        break;
    }
}

int opendrop_browser_start(opendrop_browser *browser) {
    if (!(browser->avahi_browser = avahi_service_browser_new(browser->avahi_client, browser->interface_idx, AVAHI_PROTO_INET6, "_airdrop._tcp", NULL, 0, browse_callback, browser))) {
        browser->last_avahi_error = avahi_client_errno(browser->avahi_client);
        return 1;
    }

    return 0;
}

void opendrop_browser_stop(opendrop_browser *browser) {
    if (!browser->avahi_browser) {
        return;
    }

    avahi_service_browser_free(browser->avahi_browser);

    browser->avahi_browser = NULL;
}

void opendrop_browser_set_state_callback(opendrop_browser *browser, opendrop_browser_status_cb callback, void *userdata) {
    browser->browser_status = callback;
    browser->status_userdata = userdata;
}

void opendrop_browser_set_add_service_callback(opendrop_browser *browser, opendrop_browser_service_add_cb callback, void *userdata) {
    browser->service_add = callback;
    browser->add_userdata = userdata;
}

void opendrop_browser_set_remove_service_callback(opendrop_browser *browser, opendrop_browser_service_remove_cb callback, void *userdata) {
    browser->service_remove = callback;
    browser->remove_userdata = userdata;
}

int opendrop_browser_init_errno() {
    return last_browser_init_error;
}

int opendrop_browser_errno(const opendrop_browser *browser) {
    return browser->last_avahi_error;
}

const char *opendrop_browser_strerror(int code) {
    switch (code) {
        case 1: return "Failed to start main loop.";
        case 2: return "Failed to allocate browser.";
        case 3: return "Failed to resolve network interface.";
        }

    return avahi_strerror(code);
}