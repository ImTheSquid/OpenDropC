#include <stdlib.h>
#include <net/if.h>
#include <string.h>
#include <avahi-common/thread-watch.h>
#include <avahi-client/lookup.h>
#include "../include/browser.h"

struct opendrop_browser_s {
    int sock;

    AvahiClient *avahi_client;
    unsigned int interface_idx;
    AvahiServiceBrowser *avahi_browser;

    opendrop_browser_status_cb browser_status;
    opendrop_service_add_cb service_add;
    opendrop_service_remove_cb service_remove;
    // Userdata to be passed to each callback
    void *callback_userdata;
};

// Main loop with reference count
AvahiThreadedPoll *avahi_loop;
size_t watch_refs = 0;

// Handles Avahi client events
void client_callback(AvahiClient *client, AvahiClientState state, void *userdata) {
    opendrop_browser *browser = (opendrop_browser*) userdata;

    if (state == AVAHI_CLIENT_FAILURE) {
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->callback_userdata);
    }
};

int opendrop_browser_new(opendrop_browser *browser, const char *interface) {
    // Create Avahi main loop
    if (!watch_refs) {
        avahi_loop = avahi_threaded_poll_new();
        if (!avahi_loop || avahi_threaded_poll_start(avahi_loop)) {
            return 1;
        }
    }
    watch_refs++;

    // Allocate browser struct
    browser = (opendrop_browser*) malloc(sizeof(opendrop_browser));

    // Find interface index
    if (!(browser->interface_idx = if_nametoindex(interface))) {
        opendrop_browser_free(browser);
        return 2;
    }

    // Create Avahi client
    avahi_threaded_poll_lock(avahi_loop);
    if (!(browser->avahi_client = avahi_client_new(avahi_threaded_poll_get(avahi_loop), 0, client_callback, browser, NULL))) {
        opendrop_browser_free(browser);
        avahi_threaded_poll_unlock(avahi_loop);
        return 3;
    }
    avahi_threaded_poll_unlock(avahi_loop);

    return 0;
};

void opendrop_browser_free(opendrop_browser *browser) {
    if (browser) {
        if (browser->avahi_browser) {
            avahi_service_browser_free(browser->avahi_browser);
        }

        if (browser->avahi_client) {
            avahi_client_free(browser->avahi_client);
        }
        
        free(browser);

        // Avahi loop dealloc
        if (!watch_refs || !(--watch_refs)) {
            avahi_threaded_poll_stop(avahi_loop);
            avahi_threaded_poll_free(avahi_loop);
        }
    }

    browser = NULL;
};

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
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->callback_userdata);
        break;

    case AVAHI_RESOLVER_FOUND:
        opendrop_service service;
        avahi_address_snprint(service.address, sizeof(service.address), address);
        service.name = name;
        service.type = type;
        service.domain = domain;
        service.host_name = host_name;
        service.port = port;

        (*browser->service_add)(browser, &service, browser->callback_userdata);
    }

    avahi_service_resolver_free(r);
};

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
            (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->callback_userdata);
        }
        break;

    case AVAHI_BROWSER_REMOVE:
        (*browser->service_remove)(browser, name, type, domain, browser->callback_userdata);
        break;

    case AVAHI_BROWSER_FAILURE:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_ERROR, browser->callback_userdata);
        break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_DONE, browser->callback_userdata);
        break;

    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        (*browser->browser_status)(browser, OPENDROP_BROWSER_CACHE_EMPTY, browser->callback_userdata);
        break;
    }
};

int opendrop_browser_start(opendrop_browser *browser, opendrop_service_add_cb service_add, opendrop_service_remove_cb service_remove, opendrop_browser_status_cb browser_status, void *callback_userdata) {
    browser->callback_userdata = callback_userdata;

    if (!(browser->avahi_browser = avahi_service_browser_new(browser->avahi_client, browser->interface_idx, AVAHI_PROTO_INET6, "_airdrop._tcp.local.", NULL, 0, browse_callback, browser))) {
        return 4;
    }

    return 0;
};

void opendrop_browser_stop(opendrop_browser *browser) {
    avahi_service_browser_free(browser->avahi_browser);
};