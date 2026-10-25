/* Controlled transport probe: cancel actual silent/slow TCP or TLS I/O without entering a VM. */
#define SDL_MAIN_HANDLED
#include "socket_http.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FVM_DESKTOP_HTTP
#include "http.h"

#endif
static Uint32 started;
static unsigned long after_ms;


static int cancelled(void *userdata)
{
    (void)userdata;
    return (unsigned long)(SDL_GetTicks() - started) >= after_ms;
}


static FVM_Result write_bytes(const void *bytes, size_t size, void *userdata)
{
    (void)userdata;
    return fwrite(bytes, 1, size, stdout) == size ? FVM_OK : FVM_ERROR;
}


int main(int argc, char **argv)
{
#ifndef FVM_DESKTOP_HTTP
    FVM_SocketHTTPRequest request;
#endif
    FVM_NetControl control;
    FVM_Result result;
    char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    if (argc < 3 || argc > 4)
    {
        return 2;
    }
    control.cancelled = cancelled;
    control.userdata = 0;
#ifndef FVM_DESKTOP_HTTP
    if (!fvm_desktop_socket_request_init(&request))
    {
        return 2;
    }
    request.options.total_ms = request.options.connect_ms = 3000;
    if (argc > 3)
    {
        request.options.ca_file = argv[3];
    }
    request.control = control;
#endif
    after_ms = strtoul(argv[2], 0, 10);
    started = SDL_GetTicks();
    strcpy(resolved, "unpublished");
#ifdef FVM_DESKTOP_HTTP
    result = fvm_desktop_http_fetch_controlled(argv[1], 0, write_bytes, 0, resolved, sizeof(resolved), &control);
#else
    result = fvm_desktop_socket_fetch_controlled(argv[1], 0, write_bytes, 0, resolved, sizeof(resolved), &request);
#endif
    if (result == FVM_OK || strcmp(resolved, "unpublished"))
    {
        return 1;
    }
    if (SDL_GetTicks() - started > 2000)
    {
        return 1;
    }
    SDL_Quit();
    return 0;
}
