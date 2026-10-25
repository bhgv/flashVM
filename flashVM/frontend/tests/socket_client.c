/* Binary transport probe, independent of SDL initialization and all VM runtime state. */
#include "socket_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

#endif


static FVM_Result sink(const void *bytes, size_t size, void *userdata)
{
    if (userdata)
    {
        return FVM_NOT_FOUND;
    }
    return fwrite(bytes, 1, size, stdout) == size ? FVM_OK : FVM_ERROR;
}


static int contract(void)
{
    FVM_SocketHTTPOptions options;
    char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    if (fvm_desktop_socket_options_init(0))
    {
        return 1;
    }
    if (!fvm_desktop_socket_options_init(&options) || !options.connect_ms || options.connect_ms > options.total_ms ||
        options.ca_file)
    {
        return 1;
    }
    strcpy(resolved, "unpublished");
    if (fvm_desktop_socket_fetch(0, 0, sink, 0, resolved, sizeof(resolved), 0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_socket_fetch("http://a/", 0, 0, 0, resolved, sizeof(resolved), 0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_socket_fetch("http://a/", 0, sink, 0, 0, sizeof(resolved), 0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_socket_fetch("http://a/", 0, sink, 0, resolved, 0, 0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_socket_fetch("http://a/", 0, sink, 0, resolved, 1, 0) != FVM_INVALID_ARGUMENT ||
        fvm_desktop_socket_fetch("file:///tmp/x", 0, sink, 0, resolved, sizeof(resolved), 0) != FVM_INVALID_ARGUMENT)
    {
        return 1;
    }
    options.connect_ms = 0;
    if (fvm_desktop_socket_fetch("http://a/", 0, sink, 0, resolved, sizeof(resolved), &options) != FVM_INVALID_ARGUMENT)
    {
        return 1;
    }
    options.connect_ms = options.total_ms + 1;
    if (fvm_desktop_socket_fetch("http://a/", 0, sink, 0, resolved, sizeof(resolved), &options) != FVM_INVALID_ARGUMENT)
    {
        return 1;
    }
    options.connect_ms = 1;
    options.total_ms = FVM_NET_MAX_MS + 1;
    if (fvm_desktop_socket_fetch("http://a/", 0, sink, 0, resolved, sizeof(resolved), &options) != FVM_INVALID_ARGUMENT)
    {
        return 1;
    }
    return strcmp(resolved, "unpublished") != 0;
}


int main(int argc, char **argv)
{
    FVM_SocketHTTPOptions options;
    FVM_Result result;
    char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    unsigned long timeout;
    if (argc == 2 && !strcmp(argv[1], "--contract"))
    {
        return contract();
    }
    if (argc == 2 && !strcmp(argv[1], "--unsupported-check"))
    {
        strcpy(resolved, "unpublished");
        result = fvm_desktop_socket_fetch("https://127.0.0.1:1/", 0, sink, 0, resolved, sizeof(resolved), 0);
        return result == FVM_UNSUPPORTED && !strcmp(resolved, "unpublished") ? 0 : 1;
    }
    if (argc == 2 && !strcmp(argv[1], "--capabilities"))
    {
        printf("%u\n", fvm_desktop_socket_capabilities());
        return 0;
    }
    if (argc < 2 || argc > 5)
    {
        return 2;
    }
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    fvm_desktop_socket_options_init(&options);
    if (argc > 2)
    {
        timeout = strtoul(argv[2], 0, 10);
        options.connect_ms = timeout;
        options.total_ms = timeout;
    }
    if (argc > 4)
    {
        options.ca_file = argv[4];
    }
    strcpy(resolved, "unpublished");
    result = fvm_desktop_socket_fetch(
        argv[1], 0, sink, argc > 3 && !strcmp(argv[3], "reject") ? &options : 0, resolved, sizeof(resolved), &options);
    if (result == FVM_OK)
    {
        fprintf(stderr, "canonical=%s\n", resolved);
        return 0;
    }
    if (strcmp(resolved, "unpublished"))
    {
        return 3;
    }
    fprintf(stderr, "status=%d\n", (int)result);
    return result == FVM_NOT_FOUND ? 4 : 1;
}
