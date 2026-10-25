/* Exercise the shared native resolver without external DNS or an open network connection. */
#include "network/resolver.h"
#include <stdio.h>
#include <string.h>


/* Return zero only when actual numeric lookup, bounds and failure outputs are correct.
 * No parameters, handles or allocated resources escape; errors are printed explicitly. */
int main(void)
{
    FVM_ResolverNames names;
    FVM_ResolverNames saved;
    unsigned int port = 123;
    if (fvm_port_resolve_host("127.0.0.1", &names) != FVM_OK || names.count != 1 ||
        strcmp(names.names[0], "127.0.0.1") != 0 || fvm_port_resolve_host(names.names[0], &names) != FVM_OK)
    {
        fprintf(stderr, "numeric resolver or aliased output lookup failed\n");
        return 1;
    }
    saved = names;
    if (fvm_port_resolve_address("not-an-address", &names) == FVM_OK || memcmp(&saved, &names, sizeof(names)) ||
        fvm_port_resolve_host(NULL, &names) != FVM_INVALID_ARGUMENT || memcmp(&saved, &names, sizeof(names)) ||
        fvm_port_resolve_host("", &names) != FVM_INVALID_ARGUMENT ||
        fvm_port_resolve_host("127.0.0.1", NULL) != FVM_INVALID_ARGUMENT)
    {
        fprintf(stderr, "resolver failure changed output or accepted invalid input\n");
        return 1;
    }
    if (fvm_port_resolve_service("tcp", "65536", &port) != FVM_INVALID_ARGUMENT || port != 123 ||
        fvm_port_resolve_service("tcp", "999999999999999999999999", &port) != FVM_INVALID_ARGUMENT || port != 123 ||
        fvm_port_resolve_service("invalid", "80", &port) != FVM_UNSUPPORTED || port != 123 ||
        fvm_port_resolve_service("tcp", "", &port) != FVM_INVALID_ARGUMENT || port != 123 ||
        fvm_port_resolve_service("tcp", "80", NULL) != FVM_INVALID_ARGUMENT ||
        fvm_port_resolve_service("tcp", "80", &port) != FVM_OK || port != 80 ||
        fvm_port_resolve_service("udp", "65535", &port) != FVM_OK || port != 65535 ||
        fvm_port_resolve_service("tcp", "0", &port) != FVM_OK || port != 0)
    {
        fprintf(stderr, "service port was truncated or error output changed\n");
        return 1;
    }
    puts("PASS shared numeric resolver and unchanged error outputs");
    return 0;
}
