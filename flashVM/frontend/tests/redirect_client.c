/* Counted native/curl redirect policy probe; no VM or SDL initialization required. */
#include "http_policy.h"
#include "../../ports/network/clock.h"
#ifdef FVM_DESKTOP_HTTP
#include "http.h"
#define FETCH_POLICY fvm_desktop_http_fetch_policy
#define TRANSFER_POLICY fvm_desktop_http_transfer_policy
#else
#include "socket_http.h"
#define FETCH_POLICY fvm_desktop_socket_fetch_policy
#define TRANSFER_POLICY fvm_desktop_socket_transfer_policy
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
static int cancel(void *userdata)
{
    unsigned long now;
    return !fvm_net_clock(&now) || ((now - *(unsigned long *)userdata) & FVM_NET_CLOCK_MASK) >= 40;
}
static FVM_Result sink(const void *bytes, size_t size, void *userdata)
{
    if (userdata) return FVM_NOT_FOUND;
    return fwrite(bytes, 1, size, stdout) == size ? FVM_OK : FVM_ERROR;
}
int main(int argc, char **argv)
{
    FVM_DesktopHTTPPolicy policy;
    FVM_NetControl control;
    FVM_Result result;
    unsigned long started;
    char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    if (argc == 2 && !strcmp(argv[1], "--capabilities")) {
#ifdef FVM_DESKTOP_HTTP
        printf("%u\n", fvm_desktop_http_capabilities());
#else
        printf("%u\n", fvm_desktop_socket_capabilities());
#endif
        return 0;
    }
    if (argc != 5 && argc != 6) return 2;
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (fvm_desktop_http_policy_init(0) || !fvm_desktop_http_policy_init(&policy) ||
        policy.max_redirects || !policy.connect_ms || policy.connect_ms > policy.total_ms ||
        policy.ca_file) return 3;
    policy.max_redirects = (unsigned int)strtoul(argv[2], 0, 10);
    policy.connect_ms = policy.total_ms = strtoul(argv[3], 0, 10);
    if (argc == 6) policy.ca_file = argv[5];
    strcpy(resolved, "unpublished");
    if (!strcmp(argv[4], "cancel")) {
        if (!fvm_net_clock(&started)) return 3;
        control.cancelled = cancel; control.userdata = &started;
        result = TRANSFER_POLICY(argv[1], 0, sink, 0, resolved, sizeof(resolved), &control, &policy);
    } else result = FETCH_POLICY(argv[1], 0, sink, !strcmp(argv[4], "reject") ? &policy : 0,
                                  resolved, sizeof(resolved), &policy);
    if (result == FVM_OK) fprintf(stderr, "canonical=%s\n", resolved);
    else if (strcmp(resolved, "unpublished")) return 3;
    if (!strcmp(argv[4], "reject")) return result == FVM_NOT_FOUND ? 4 : 3;
    return result == FVM_OK ? 0 : 1;
}
