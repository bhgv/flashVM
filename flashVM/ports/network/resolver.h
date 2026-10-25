/* Shared synchronous hostname/service resolution; no VM or Inferno OS dependency. */
#ifndef FVM_NETWORK_RESOLVER_H
#define FVM_NETWORK_RESOLVER_H
#include "flashvm/flashvm.h"

#define FVM_RESOLVER_NAMES 16       /* Maximum distinct answers returned by one native lookup. */
#define FVM_RESOLVER_TEXT 1025      /* Bytes per NUL-terminated numeric address or host name. */
#define FVM_RESOLVER_PORT_MAX 65535 /* Largest accepted transport service port. */

#ifdef __cplusplus
extern "C"
{
#endif


/* Caller-owned bounded result; no allocated resources escape a resolver call. */
typedef struct FVM_ResolverNames
{
    size_t count;                                      /* Number of initialized distinct answer strings. */
    char names[FVM_RESOLVER_NAMES][FVM_RESOLVER_TEXT]; /* UTF-8/native ASCII names, each terminated within its slot. */
} FVM_ResolverNames;


/* Resolve borrowed host text to numeric IPv4/IPv6 addresses. out changes only
 * on success; no native resources escape. May block in the OS resolver; no timeout. */
FVM_Result fvm_port_resolve_host(const char *host, FVM_ResolverNames *out);


/* Reverse a borrowed numeric address to one required host name. out changes only
 * on success; invalid/unresolved names fail explicitly. May block in native DNS. */
FVM_Result fvm_port_resolve_address(const char *address, FVM_ResolverNames *out);


/* Resolve borrowed tcp/udp service text or checked decimal port. port changes only
 * on success; no handles escape. Other network names are rejected. May block. */
FVM_Result fvm_port_resolve_service(const char *network, const char *service, unsigned int *port);

#ifdef __cplusplus
}
#endif
#endif
