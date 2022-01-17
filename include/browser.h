#include <stdint.h>
#include <stdbool.h>

typedef struct opendrop_browser_s opendrop_browser;

typedef enum opendrop_browser_status_e {
    OPENDROP_BROWSER_DONE, // No more services, probably for a while
    OPENDROP_BROWSER_CACHE_EMPTY, // No more services to send
    OPENDROP_BROWSER_ERROR // Avahi browser error
} opendrop_browser_status;

// Structure for OpenDrop service within a list
typedef struct opendrop_service_s {
    const char *name;
    const char *type;
    const char *domain;
    const char *host_name;
    uint16_t port;

    unsigned char address[16];
} opendrop_service;

// Callback for added services
// Args:
// - Browser instance
// - Service struct
// - Userdata
typedef void (*opendrop_browser_service_add_cb)(opendrop_browser*, const opendrop_service*, void*);

// Callback for removed services
// Args:
// - Browser instance
// - Service name
// - Service type
// - Service domain
// - Userdata
typedef void (*opendrop_browser_service_remove_cb)(opendrop_browser*, const char*, const char*, const char*, void*);

// Callback for browser status
// Args:
// - Browser instance
// - Browser status
// - Userdata
typedef void (*opendrop_browser_status_cb)(opendrop_browser*, opendrop_browser_status, void*);

// Initializes OpenDrop browser
// Args:
// - browser: OpenDrop browser
// - interface: The NULL-terminated name of the interface to bind the browser to
// Returns 0 on success, >0 on error
int opendrop_browser_new(opendrop_browser **browser, const char *interface);

// Frees OpenDrop browser, memory will become invalid
// Args:
// - browser: OpenDrop browser
void opendrop_browser_free(opendrop_browser *browser);

// Starts OpenDrop browser
// Args:
// - browser: Initialized browser
// - service_add: Callback for when a service is added
// - service_remove: Callback for when a service is removed
// - browser_status: Callback for change of browser status
// - callback_userdata: Data to be passed to all callbacks
// Returns 0 on success, >0 on error
int opendrop_browser_start(opendrop_browser *browser);

// Stops OpenDrop browser
// Args:
// - browser: OpenDrop browser
void opendrop_browser_stop(opendrop_browser *browser);

// Sets state callback
// Args:
// - browser: OpenDrop browser
// - callback: State callback
// - userdata: Data to be passed to callback
void opendrop_browser_set_state_callback(opendrop_browser *browser, opendrop_browser_status_cb callback, void *userdata);

// Sets add service callback
// Args:
// - browser: OpenDrop browser
// - callback: State callback
// - userdata: Data to be passed to callback
void opendrop_browser_set_add_service_callback(opendrop_browser *browser, opendrop_browser_service_add_cb callback, void *userdata);

// Sets remove service callback
// Args:
// - browser: OpenDrop browser
// - callback: State callback
// - userdata: Data to be passed to callback
void opendrop_browser_set_remove_service_callback(opendrop_browser *browser, opendrop_browser_service_remove_cb callback, void *userdata);

// Gets the previous initialization error code
int opendrop_browser_init_errno();

// Gets the previous error code
// Args:
// - browser: OpenDrop browser
int opendrop_browser_errno(const opendrop_browser *browser);

// Gets string description from error code
// Args:
// - code: Error code
// - init: Whether error was from init or browser
const char *opendrop_browser_strerror(int code);