/* Strict bounded HTTP/1.x over the native TCP adapter. No socket operation invokes the VM. */
#include "socket_http.h"
#include "socket_url.h"
#include <string.h>
#include <stdlib.h>
#ifdef FVM_DESKTOP_SOCKET_TLS
#include "../../ports/network/tls.h"
#endif

/* One request owns the socket and bounded parsing storage until cleanup. */
typedef struct FVM_SocketReader
{
    FVM_NetSocket *socket; /* Owned TCP connection, closed by the provider. */
#ifdef FVM_DESKTOP_SOCKET_TLS
    FVM_NetTLS *tls; /* Owned optional TLS layer, released before the TCP handle. */
#endif
    unsigned char bytes[FVM_SOCKET_BUFFER]; /* Unconsumed native read bytes. */
    size_t position;                        /* First unconsumed buffered byte. */
    size_t size;                            /* Valid buffered byte count. */
    unsigned long header_bytes;             /* Shared informational/header/trailer budget. */
    unsigned int fields;                    /* Aggregate field-work budget. */
    unsigned long body_bytes;               /* Total forwarded decoded body bytes. */
    FVM_DesktopWrite write;                 /* Borrowed sink, never retained after return. */
    void *userdata;                         /* Borrowed sink state. */
} FVM_SocketReader;
/* Framing is explicit: a transfer coding never silently overrides an ambiguous length. */
typedef struct FVM_SocketFraming
{
    unsigned long length; /* Validated decimal length, when length_seen is true. */
    int length_seen;      /* Reject duplicate Content-Length even when identical. */
    int chunked;          /* Exactly one supported Transfer-Encoding: chunked field. */
} FVM_SocketFraming;

/* ASCII-only folding and token recognition avoid locale-sensitive HTTP parsing. */
static char http_lower(char c)
{
    return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}
/* Exact case-insensitive match for protocol field names and supported coding tokens. */
static int http_equal(const char *a, const char *b)
{
    while (*a && *b)
        if (http_lower(*a++) != http_lower(*b++))
            return 0;
    return !*a && !*b;
}
/* RFC token punctuation plus ASCII alphanumerics, excluding delimiters and whitespace. */
static int http_token(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           (c && strchr("!#$%&'*+-.^_`|~", c));
}
/* Route one read through the verified TLS layer when present, otherwise native TCP. */
static int http_receive(FVM_SocketReader *r, void *bytes, size_t size, size_t *received)
{
#ifdef FVM_DESKTOP_SOCKET_TLS
    if (r->tls)
        return fvm_net_tls_read(r->tls, bytes, size, received);
#endif
    return fvm_net_read(r->socket, bytes, size, received);
}
/* Route one counted write without exposing platform or TLS handles to protocol parsing. */
static int http_send(FVM_SocketReader *r, const void *bytes, size_t size, size_t *sent)
{
#ifdef FVM_DESKTOP_SOCKET_TLS
    if (r->tls)
        return fvm_net_tls_write(r->tls, bytes, size, sent);
#endif
    return fvm_net_write(r->socket, bytes, size, sent);
}
/* Refill only after consuming the previous chunk; distinguish clean EOF from native error. */
static int http_fill(FVM_SocketReader *r)
{
    if (r->position < r->size)
        return 1;
    r->position = 0;
    if (!http_receive(r, r->bytes, sizeof(r->bytes), &r->size))
        return -1;
    return r->size ? 1 : 0;
}
/* Read a CRLF line; bare LF/NUL/control bytes and unbounded metadata are rejected. */
static int http_line(FVM_SocketReader *r, char *line, int metadata)
{
    size_t size = 0;
    unsigned char c;
    int cr = 0;
    for (;;)
    {
        if (http_fill(r) != 1)
            return 0;
        c = r->bytes[r->position++];
        if (metadata && ++r->header_bytes > FVM_SOCKET_HEADERS)
            return 0;
        if (cr)
        {
            if (c != '\n')
                return 0;
            line[size] = 0;
            return 1;
        }
        if (c == '\r')
        {
            cr = 1;
            continue;
        }
        if (c == '\n' || !c || (c < ' ' && c != '\t') || c == FVM_SOCKET_ASCII_DEL ||
            size >= FVM_SOCKET_LINE)
            return 0;
        line[size++] = (char)c;
    }
}
/* Parse a bounded unsigned number without overflow, signs, padding or locale coercion. */
static int http_decimal(const char *text, unsigned long *out)
{
    unsigned long value = 0, digit;
    if (!*text)
        return 0;
    while (*text)
    {
        if (*text < '0' || *text > '9')
            return 0;
        digit = (unsigned long)(*text++ - '0');
        if (value > (FVM_SOCKET_BODY_MAX - digit) / FVM_SOCKET_DECIMAL_RADIX)
            return 0;
        value = value * FVM_SOCKET_DECIMAL_RADIX + digit;
    }
    *out = value;
    return 1;
}
/* Parse one field and forbid obsolete folding or whitespace before the colon. */
static int http_field(FVM_SocketReader *r, char *line, FVM_SocketFraming *framing, int trailer)
{
    char *p = line, *value, *end;
    if (++r->fields > FVM_SOCKET_FIELDS || !http_token(*p))
        return 0;
    while (http_token(*p))
        ++p;
    if (*p != ':')
        return 0;
    *p++ = 0;
    while (*p == ' ' || *p == '\t')
        ++p;
    value = p;
    end = value + strlen(value);
    while (end > value && (end[-1] == ' ' || end[-1] == '\t'))
        --end;
    *end = 0;
    if (http_equal(line, "content-length"))
    {
        if (trailer || framing->length_seen || !http_decimal(value, &framing->length))
            return 0;
        framing->length_seen = 1;
    }
    else if (http_equal(line, "transfer-encoding"))
    {
        if (trailer || framing->chunked || !http_equal(value, "chunked"))
            return 0;
        framing->chunked = 1;
    }
    else if (http_equal(line, "content-encoding"))
    {
        if (trailer || !http_equal(value, "identity"))
            return 0;
    }
    return 1;
}
/* Parse interim responses within the same global metadata budget; upgrades are unsupported. */
static int http_headers(FVM_SocketReader *r, FVM_SocketFraming *framing, int *status)
{
    char line[FVM_SOCKET_LINE + 1];
    unsigned int informational = 0;
    size_t size;
    for (;;)
    {
        memset(framing, 0, sizeof(*framing));
        if (!http_line(r, line, 1))
            return 0;
        size = strlen(line);
        if (size < 12 || (strncmp(line, "HTTP/1.0 ", 9) && strncmp(line, "HTTP/1.1 ", 9)) ||
            line[9] < '1' || line[9] > '5' || line[10] < '0' || line[10] > '9' || line[11] < '0' ||
            line[11] > '9' || (size > 12 && line[12] != ' '))
            return 0;
        *status = (line[9] - '0') * 100 + (line[10] - '0') * 10 + (line[11] - '0');
        for (;;)
        {
            if (!http_line(r, line, 1))
                return 0;
            if (!*line)
                break;
            if (!http_field(r, line, framing, 0))
                return 0;
        }
        if (framing->chunked && framing->length_seen)
            return 0;
        if (*status >= FVM_SOCKET_STATUS_MIN)
            return 1;
        if (*status < FVM_SOCKET_STATUS_CONTINUE || *status == FVM_SOCKET_STATUS_SWITCH ||
            framing->chunked || framing->length_seen || ++informational > FVM_SOCKET_INFORMATIONAL)
            return 0;
    }
}
/* Forward decoded bytes and preserve the sink's exact error without further callback attempts. */
static FVM_Result http_forward(FVM_SocketReader *r, size_t size)
{
    FVM_Result result;
    if (size > FVM_SOCKET_BODY_MAX - r->body_bytes)
        return FVM_INVALID_ARGUMENT;
    result = r->write(r->bytes + r->position, size, r->userdata);
    if (result != FVM_OK)
        return result;
    r->position += size;
    r->body_bytes += (unsigned long)size;
    return FVM_OK;
}
/* Consume exactly one framed slice; premature EOF is never a successful resource. */
static FVM_Result http_exact(FVM_SocketReader *r, unsigned long size)
{
    size_t available;
    FVM_Result result;
    if (size > FVM_SOCKET_BODY_MAX - r->body_bytes)
        return FVM_INVALID_ARGUMENT;
    while (size)
    {
        if (http_fill(r) != 1)
            return FVM_ERROR;
        available = r->size - r->position;
        if (available > size)
            available = (size_t)size;
        result = http_forward(r, available);
        if (result != FVM_OK)
            return result;
        size -= (unsigned long)available;
    }
    return FVM_OK;
}
/* Validate chunk extensions, including quoted strings, without interpreting their values. */
static int http_extensions(const char *p)
{
    while (*p)
    {
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p++ != ';')
            return 0;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (!http_token(*p))
            return 0;
        while (http_token(*p))
            ++p;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (*p == '=')
        {
            ++p;
            while (*p == ' ' || *p == '\t')
                ++p;
            if (*p == '"')
            {
                ++p;
                while (*p && *p != '"')
                {
                    if (*p == '\\')
                    {
                        ++p;
                        if (!*p)
                            return 0;
                    }
                    ++p;
                }
                if (*p++ != '"')
                    return 0;
            }
            else
            {
                if (!http_token(*p))
                    return 0;
                while (http_token(*p))
                    ++p;
            }
        }
    }
    return 1;
}
/* Hexadecimal chunk size must fit both the resource bound and remaining decoded-body budget. */
static int http_chunk_size(const char *p, unsigned long *out)
{
    unsigned long value = 0, digit;
    int seen = 0;
    for (;;)
    {
        if (*p >= '0' && *p <= '9')
            digit = (unsigned long)(*p - '0');
        else if (http_lower(*p) >= 'a' && http_lower(*p) <= 'f')
            digit = (unsigned long)(http_lower(*p) - 'a' + 10);
        else
            break;
        seen = 1;
        ++p;
        if (value > (FVM_SOCKET_BODY_MAX - digit) / FVM_SOCKET_HEX_RADIX)
            return 0;
        value = value * FVM_SOCKET_HEX_RADIX + digit;
    }
    if (!seen || (*p && !http_extensions(p)))
        return 0;
    *out = value;
    return 1;
}
/* Decode one chunked message, validating the final CRLF and bounded trailers before success. */
static FVM_Result http_chunked(FVM_SocketReader *r, FVM_SocketFraming *framing)
{
    char line[FVM_SOCKET_LINE + 1];
    unsigned long size, chunks = 0;
    FVM_Result result;
    for (;;)
    {
        if (++chunks > FVM_SOCKET_CHUNKS || !http_line(r, line, 0) || !http_chunk_size(line, &size))
            return FVM_ERROR;
        if (!size)
            break;
        result = http_exact(r, size);
        if (result != FVM_OK)
            return result;
        if (!http_line(r, line, 0) || *line)
            return FVM_ERROR;
    }
    for (;;)
    {
        if (!http_line(r, line, 1))
            return FVM_ERROR;
        if (!*line)
            return FVM_OK;
        if (!http_field(r, line, framing, 1))
            return FVM_ERROR;
    }
}
/* Send the complete bounded request despite arbitrary partial native writes. */
static int http_request(FVM_SocketReader *reader, const FVM_SocketURL *url)
{
    char text[FVM_DESKTOP_MAX_LOCATION * 2 + FVM_SOCKET_REQUEST_OVERHEAD];
    size_t remaining, sent;
    const char *p;
    strcpy(text, "GET ");
    strcat(text, url->target);
    strcat(text, " HTTP/1.1\r\nHost: ");
    strcat(text, url->authority);
    strcat(text, "\r\nConnection: close\r\nAccept-Encoding: identity\r\n\r\n");
    p = text;
    remaining = strlen(text);
    while (remaining)
    {
        if (!http_send(reader, p, remaining, &sent) || !sent)
            return 0;
        p += sent;
        remaining -= sent;
    }
    return 1;
}
/* Construct default per-request policy without allocating or touching platform state. */
int fvm_desktop_socket_options_init(FVM_SocketHTTPOptions *out)
{
    if (!out)
        return 0;
    out->connect_ms = FVM_NET_CONNECT_MS;
    out->total_ms = FVM_NET_TOTAL_MS;
    out->ca_file = 0;
    return 1;
}
/* Resolve, fetch, validate framing, deliver bounded bytes, then publish canonical provenance. */
FVM_Result fvm_desktop_socket_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                                    void *write_userdata, char *resolved, size_t capacity,
                                    void *userdata)
{
    FVM_SocketURL url;
    FVM_SocketHTTPOptions defaults;
    const FVM_SocketHTTPOptions *options = (const FVM_SocketHTTPOptions *)userdata;
    FVM_SocketReader reader;
    FVM_SocketFraming framing;
    FVM_Result result;
    int status, available;
    if (!reference || !write || !resolved || !capacity)
        return FVM_INVALID_ARGUMENT;
    if (!options)
    {
        fvm_desktop_socket_options_init(&defaults);
        options = &defaults;
    }
    if (!options->connect_ms || options->connect_ms > options->total_ms ||
        options->total_ms > FVM_NET_MAX_MS)
        return FVM_INVALID_ARGUMENT;
    result = fvm_socket_url_resolve(reference, base, &url);
    if (result != FVM_OK)
        return result;
    if (strlen(url.canonical) >= capacity)
        return FVM_INVALID_ARGUMENT;
#ifndef FVM_DESKTOP_SOCKET_TLS
    if (url.tls)
        return FVM_UNSUPPORTED;
#endif
    memset(&reader, 0, sizeof(reader));
    reader.write = write;
    reader.userdata = write_userdata;
    reader.socket = fvm_net_open(url.host, url.port, options->connect_ms, options->total_ms);
    if (!reader.socket)
        return FVM_ERROR;
    result = FVM_ERROR;
#ifdef FVM_DESKTOP_SOCKET_TLS
    if (url.tls)
    {
        reader.tls = fvm_net_tls_open(reader.socket, url.host, url.numeric, options->ca_file);
        if (!reader.tls)
            goto cleanup;
    }
#endif
    if (!http_request(&reader, &url) || !http_headers(&reader, &framing, &status) ||
        status < FVM_SOCKET_STATUS_MIN || status >= FVM_SOCKET_STATUS_END)
        goto cleanup;
    if (status == FVM_SOCKET_STATUS_EMPTY || status == FVM_SOCKET_STATUS_RESET)
    {
        if (framing.chunked || (framing.length_seen && framing.length))
            goto cleanup;
        result = FVM_OK;
    }
    else if (framing.chunked)
        result = http_chunked(&reader, &framing);
    else if (framing.length_seen)
        result = http_exact(&reader, framing.length);
    else
    {
        for (;;)
        {
            available = http_fill(&reader);
            if (available < 0)
                break;
            if (!available)
            {
                result = FVM_OK;
                break;
            }
            result = http_forward(&reader, reader.size - reader.position);
            if (result != FVM_OK)
                break;
            result = FVM_ERROR;
        }
    }
    if (result == FVM_OK)
        memcpy(resolved, url.canonical, strlen(url.canonical) + 1);
cleanup:
#ifdef FVM_DESKTOP_SOCKET_TLS
    fvm_net_tls_close(reader.tls);
#endif
    fvm_net_close(reader.socket);
    return result;
}

/* Build discovery performs no socket, TLS, trust, resolver or VM initialization. */
unsigned int fvm_desktop_socket_capabilities(void)
{
    unsigned int flags = FVM_DESKTOP_NETWORK_HTTP | FVM_DESKTOP_NETWORK_SOCKET;
#ifdef FVM_DESKTOP_SOCKET_TLS
    flags |= FVM_DESKTOP_NETWORK_HTTPS;
#endif
    return flags;
}
