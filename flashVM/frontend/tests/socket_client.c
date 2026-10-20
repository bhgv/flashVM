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
        return FVM_NOT_FOUND;
    return fwrite(bytes, 1, size, stdout) == size ? FVM_OK : FVM_ERROR;
}
int main(int argc, char **argv)
{
    FVM_SocketHTTPOptions options;
    FVM_Result result;
    char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    unsigned long timeout;
    if (argc < 2 || argc > 4)
        return 2;
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
    strcpy(resolved, "unpublished");
    result = fvm_desktop_socket_fetch(argv[1], 0, sink, argc > 3 ? &options : 0, resolved,
                                      sizeof(resolved), &options);
    if (result == FVM_OK)
    {
        fprintf(stderr, "canonical=%s\n", resolved);
        return 0;
    }
    if (strcmp(resolved, "unpublished"))
        return 3;
    fprintf(stderr, "status=%d\n", (int)result);
    return result == FVM_NOT_FOUND ? 4 : 1;
}
