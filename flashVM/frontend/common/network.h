/* Default player transport discovery without changing the existing build-info struct ABI. */
#ifndef FVM_DESKTOP_NETWORK_H
#define FVM_DESKTOP_NETWORK_H
#include "export.h"

#define FVM_DESKTOP_NETWORK_HTTP 1U   /* Default provider implements HTTP resource GET. */
#define FVM_DESKTOP_NETWORK_HTTPS 2U  /* Default provider implements verified HTTPS GET. */
#define FVM_DESKTOP_NETWORK_SOCKET 4U /* Provider uses the native POSIX/Winsock TCP adapter. */
#define FVM_DESKTOP_NETWORK_CURL 8U   /* Provider uses the linked libcurl SDK. */
#define FVM_DESKTOP_NETWORK_MASK 15U  /* All currently defined capability bits. */
#ifdef __cplusplus
extern "C"
{
#endif


/* Return compiled default-provider bits, or zero when none is configured. No network,
     * file, SDL, VM or GC startup. This is not connectivity/trust/device readiness and
     * does not describe a custom loader.fetch callback. Callable before playback on the
     * host thread. With older libcurl, coordinate concurrent curl initialization/query
     * according to that SDK's thread rules. No pointers or mutable registration state. */
FVM_DESKTOP_API unsigned int fvm_desktop_network_capabilities(void);
#ifdef __cplusplus
}
#endif
#endif
