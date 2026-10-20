/* Native socket SWF host; API contracts and build instructions: README.md / README.ru.md. */
#include "../../../frontend/common/player.h" /* Caller-owned player configuration and source API. */
#include "../../../frontend/common/socket_http.h" /* Native TCP transport; no libcurl required. */
#include <stdio.h> /* Report the static host error without owning its storage. */
#define EXAMPLE_SOCKET_LOADS 8UL /* Startup plus child movies/forms share this request budget. */
#define EXAMPLE_SOCKET_FRAMES 3UL /* Finish this tutorial after three movie ticks. */
#define EXAMPLE_SOCKET_CONNECT 2000UL /* TCP connection budget in milliseconds. */
#define EXAMPLE_SOCKET_TOTAL 5000UL /* Total native I/O budget for each resource. */

/* Run on the native UI thread; the socket callback is synchronous at player safe points. */
int main(int argc, char **argv) /* Supply one absolute HTTP URL, including the movie filename. */
{ /* Begin the lifetime of all borrowed source and callback storage. */
    FVM_DesktopOptions options; /* Scalar player settings are initialized before use. */
    FVM_DesktopSource source; /* The URL is borrowed from argv until synchronous return. */
    FVM_DesktopLoader loader; /* Explicit provider selection also works in an IDE host. */
    FVM_SocketHTTPOptions network; /* No hidden process-global timeout configuration. */
    const char *error = 0; /* Playback may return a static, non-owned explanation. */
    int status; /* Preserve the player exit code for a calling shell/application. */
    if (argc != 2) /* The tutorial needs one source, not a player CLI argument list. */
    { /* Reject an absent or ambiguous source before opening any platform resource. */
        fprintf(stderr, "Usage: %s http://host/path/start.swf\n", argv[0]); /* Show URL syntax. */
        return 2; /* Match the player's argument-error convention. */
    } /* Valid command-line shape; URI validation belongs to the provider. */
    fvm_desktop_options_init(&options); /* Initialize every option using the shared C API. */
    fvm_desktop_loader_init(&loader); /* Initialize callbacks and the load budget. */
    fvm_desktop_socket_options_init(&network); /* Install documented TCP/time defaults. */
    network.connect_ms = EXAMPLE_SOCKET_CONNECT; /* DNS may still block inside the OS resolver. */
    network.total_ms = EXAMPLE_SOCKET_TOTAL; /* One deadline covers partial headers and body. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* Work without a compiled native JIT backend. */
    options.mute = 1; /* This tutorial opens no audio endpoint. */
    options.frames = EXAMPLE_SOCKET_FRAMES; /* Bound the event/render loop for automation. */
    loader.fetch = fvm_desktop_socket_fetch; /* Native POSIX/Winsock provider, not libcurl. */
    loader.userdata = &network; /* Borrowed until fvm_desktop_run_source returns. */
    loader.max_loads = EXAMPLE_SOCKET_LOADS; /* Permit SWF-initiated movie and variable requests. */
    source.kind = FVM_DESKTOP_SOURCE_URL; /* Fetch the startup as a URL resource. */
    source.bytes = 0; /* URL input does not borrow an initial SWF byte array. */
    source.size = 0; /* The provider determines the counted resource size. */
    source.location = argv[1]; /* Effective canonical URL becomes each movie's resource base. */
    status = fvm_desktop_run_source(&options, &source, &loader, &error); /* Own-thread playback. */
    if (status && error) fprintf(stderr, "%s\n", error); /* Never free the static error. */
    fvm_desktop_shutdown(); /* Release the matching library session after player cleanup. */
    return status; /* All request sockets and temporary resources have been released. */
} /* No callbacks retain these stack-owned descriptors. */
