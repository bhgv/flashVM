/* Internal HTTP URI resolver shared by the socket provider and parser fixtures. */
#ifndef FVM_SOCKET_URL_H
#define FVM_SOCKET_URL_H
#include "source.h"

#define FVM_SOCKET_HTTP_PORT 80UL     /* Plain HTTP default service. */
#define FVM_SOCKET_HTTPS_PORT 443UL   /* TLS HTTP default service. */
#define FVM_SOCKET_PORT_MAX 65535UL   /* Largest usable TCP port. */
#define FVM_SOCKET_PORT_TEXT 6U       /* Five decimal digits and terminator. */
#define FVM_SOCKET_HEX_RADIX 16U      /* Percent-escape digit radix. */
#define FVM_SOCKET_DECIMAL_RADIX 10UL /* Port and Content-Length radix. */
#define FVM_SOCKET_ASCII_DEL 127U     /* First forbidden ASCII control after printable bytes. */


/* Fixed bounds avoid allocation during validation; canonical includes no fragment or credentials.
 */
typedef struct FVM_SocketURL
{
    char canonical[FVM_DESKTOP_MAX_LOCATION + 1]; /* Effective provenance on successful fetch. */
    char host[FVM_DESKTOP_MAX_LOCATION + 1];      /* DNS name or unbracketed IPv6 literal. */
    char authority[FVM_DESKTOP_MAX_LOCATION + 1]; /* Validated Host field, with port when needed. */
    char target[FVM_DESKTOP_MAX_LOCATION + 1];    /* Origin-form path and optional query. */
    char port[FVM_SOCKET_PORT_TEXT];              /* Normalized numeric TCP service. */
    int tls;                                      /* https requires a verified TLS adapter. */
    int numeric;                                  /* IPv4/IPv6 spelling for certificate verification. */
} FVM_SocketURL;


/* Resolve an ASCII URI reference using literal dot-segment removal; reject userinfo,
 * controls, spaces, backslashes, malformed percent escapes and non-HTTP(S) schemes.
 * Output may be modified on failure. No input pointer is retained. */
FVM_Result fvm_socket_url_resolve(const char *reference, const char *base, FVM_SocketURL *out);
#endif
