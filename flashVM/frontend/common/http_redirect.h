/* Internal common redirect orchestration, bounded header extraction and provider bridge. */
#ifndef FVM_HTTP_REDIRECT_H
#define FVM_HTTP_REDIRECT_H
#include "http_policy.h"
#include "socket_url.h"


/* One response's metadata is owned by the synchronous chain, never shared with VM objects. */
typedef struct FVM_HTTPResponse
{
    int status;        /* Final or current informational HTTP status. */
    int location_seen; /* Single Location only; duplicates never silently choose a destination. */
    char location[FVM_DESKTOP_MAX_LOCATION + 1]; /* Bounded raw URI reference for one hop. */
    size_t header_bytes;                         /* Aggregate curl header accounting, reset only for the next hop. */
    unsigned int fields;                         /* Aggregate metadata work including informational response blocks. */
} FVM_HTTPResponse;


/* One transport hop must deliver only successful final body bytes; redirect bodies are discarded.
 * A redirect returns OK with status/Location and no body. Caller owns every descriptor. */
typedef FVM_Result (*FVM_HTTPHop)(const FVM_SocketURL *url,
                                  FVM_DesktopWrite write,
                                  void *sink,
                                  const FVM_DesktopHTTPPolicy *policy,
                                  const FVM_NetControl *control,
                                  FVM_HTTPResponse *response);


/* Return nonzero only for the supported GET redirect status codes. */
int fvm_http_redirect_status(int status);


/* Validate/copy exactly one Location reference; no implicit list splitting or embedded controls. */
int fvm_http_redirect_location(FVM_HTTPResponse *response, const char *value, size_t size);


/* Parse one counted CRLF curl header line; no input pointer is retained. */
int fvm_http_redirect_header(FVM_HTTPResponse *response, const char *bytes, size_t size);


/* Validate policy, follow bounded HTTP(S) GET hops, retain one deadline/cancel chain and publish
 * canonical provenance only after final success. No downgrade from HTTPS to plain HTTP. */
FVM_Result fvm_http_redirect_fetch(const char *reference,
                                   const char *base,
                                   FVM_DesktopWrite write,
                                   void *sink,
                                   char *resolved,
                                   size_t capacity,
                                   const FVM_DesktopHTTPPolicy *policy,
                                   const FVM_NetControl *control,
                                   FVM_HTTPHop hop);
#endif
