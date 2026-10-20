/* Bounded RFC3986-style HTTP reference resolution, without URI-library or locale dependencies. */
#include "socket_url.h"
#include <string.h>

/* ASCII case folding is independent of the process locale. */
static char url_lower(char c)
{
    return c >= 'A' && c <= 'Z' ? (char)(c + ('a' - 'A')) : c;
}
/* Match a scheme prefix without reading past a terminating NUL. */
static int url_prefix(const char *text, const char *prefix)
{
    while (*prefix)
        if (url_lower(*text++) != *prefix++)
            return 0;
    return 1;
}
/* URI input must already be escaped ASCII; encoded bytes are preserved, not interpreted. */
static int url_valid(const char *text)
{
    size_t i;
    unsigned char c;
    for (i = 0; text[i]; ++i)
    {
        c = (unsigned char)text[i];
        if (i >= FVM_DESKTOP_MAX_LOCATION || c <= ' ' || c >= FVM_SOCKET_ASCII_DEL || c == '\\')
            return 0;
        if (c == '%')
        {
            if (!text[i + 1] || !text[i + 2] || !strchr("0123456789abcdefABCDEF", text[i + 1]) ||
                !strchr("0123456789abcdefABCDEF", text[i + 2]))
                return 0;
            i += 2;
            if (i >= FVM_DESKTOP_MAX_LOCATION)
                return 0;
        }
    }
    return 1;
}
/* Copy one bounded slice, including its own terminator. */
static int url_copy(char *out, const char *begin, size_t size)
{
    if (size > FVM_DESKTOP_MAX_LOCATION)
        return 0;
    memcpy(out, begin, size);
    out[size] = 0;
    return 1;
}
/* Append a slice to a maximum-location buffer without truncation. */
static int url_append(char *out, const char *text, size_t size)
{
    size_t used = strlen(out);
    if (size > FVM_DESKTOP_MAX_LOCATION - used)
        return 0;
    memcpy(out + used, text, size);
    out[used + size] = 0;
    return 1;
}
/* Remove the last complete path segment from an output prefix. */
static void url_pop(char *path)
{
    char *slash = strrchr(path, '/');
    if (slash)
        *slash = 0;
    else
        *path = 0;
}
/* RFC3986 section 5.2.4 literal dot segments; repeated slashes and escaped dots survive. */
static void url_normalize(char *path)
{
    char pending[FVM_DESKTOP_MAX_LOCATION + 1], output[FVM_DESKTOP_MAX_LOCATION + 1];
    char *p, *end;
    strcpy(pending, path);
    output[0] = 0;
    p = pending;
    while (*p)
    {
        if (!strncmp(p, "../", 3))
            p += 3;
        else if (!strncmp(p, "./", 2))
            p += 2;
        else if (!strncmp(p, "/./", 3))
            p += 2;
        else if (!strcmp(p, "/."))
        {
            p[1] = 0;
        }
        else if (!strncmp(p, "/../", 4))
        {
            p += 3;
            url_pop(output);
        }
        else if (!strcmp(p, "/.."))
        {
            p[1] = 0;
            url_pop(output);
        }
        else if (!strcmp(p, ".") || !strcmp(p, ".."))
            *p = 0;
        else
        {
            end = strchr(p + (*p == '/' ? 1 : 0), '/');
            if (!end)
                end = p + strlen(p);
            /* Normalization cannot grow beyond the original path length. */
            url_append(output, p, (size_t)(end - p));
            p = end;
        }
    }
    strcpy(path, output);
}
/* Format a previously bounded TCP port without printf width or integer-range assumptions. */
static void url_port(char *out, unsigned long number)
{
    char digits[FVM_SOCKET_PORT_TEXT];
    size_t count = 0, i;
    do
    {
        digits[count++] = (char)('0' + number % FVM_SOCKET_DECIMAL_RADIX);
        number /= FVM_SOCKET_DECIMAL_RADIX;
    } while (number);
    for (i = 0; i < count; ++i)
        out[i] = digits[count - i - 1];
    out[count] = 0;
}
/* Parse an absolute URL into validated host/service/path fields; caller resolves relative input. */
static int url_absolute(const char *input, FVM_SocketURL *out)
{
    const char *begin, *end, *host_end, *port = 0, *p, *query;
    unsigned long number, default_port;
    size_t i, host_size;
    char path[FVM_DESKTOP_MAX_LOCATION + 1];
    if (url_prefix(input, "http://"))
    {
        out->tls = 0;
        begin = input + 7;
    }
    else if (url_prefix(input, "https://"))
    {
        out->tls = 1;
        begin = input + 8;
    }
    else
        return 0;
    end = begin + strcspn(begin, "/?#");
    if (end == begin || memchr(begin, '@', (size_t)(end - begin)))
        return 0;
    out->numeric = 0;
    if (*begin == '[')
    {
        host_end = (const char *)memchr(begin, ']', (size_t)(end - begin));
        if (!host_end || host_end == begin + 1)
            return 0;
        for (p = begin + 1; p != host_end; ++p)
            if (!strchr("0123456789abcdefABCDEF:.", *p))
                return 0;
        if (!memchr(begin, ':', (size_t)(host_end - begin)))
            return 0;
        if (host_end + 1 != end)
        {
            if (host_end[1] != ':')
                return 0;
            port = host_end + 2;
        }
        ++begin;
        out->numeric = 1;
    }
    else
    {
        host_end = (const char *)memchr(begin, ':', (size_t)(end - begin));
        if (!host_end)
            host_end = end;
        else
            port = host_end + 1;
        if (host_end == begin)
            return 0;
        out->numeric = 1;
        for (p = begin; p != host_end; ++p)
        {
            if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
                  (*p >= '0' && *p <= '9') || *p == '-' || *p == '.'))
                return 0;
            if (!((*p >= '0' && *p <= '9') || *p == '.'))
                out->numeric = 0;
        }
    }
    host_size = (size_t)(host_end - begin);
    if (!url_copy(out->host, begin, host_size))
        return 0;
    for (i = 0; i < host_size; ++i)
        out->host[i] = url_lower(out->host[i]);
    default_port = out->tls ? FVM_SOCKET_HTTPS_PORT : FVM_SOCKET_HTTP_PORT;
    number = default_port;
    if (port)
    {
        if (port == end)
            return 0;
        number = 0;
        for (p = port; p != end; ++p)
        {
            if (*p < '0' || *p > '9' ||
                number >
                    (FVM_SOCKET_PORT_MAX - (unsigned long)(*p - '0')) / FVM_SOCKET_DECIMAL_RADIX)
                return 0;
            number = number * FVM_SOCKET_DECIMAL_RADIX + (unsigned long)(*p - '0');
        }
        if (!number)
            return 0;
    }
    url_port(out->port, number);
    out->authority[0] = 0;
    if (out->numeric && strchr(out->host, ':'))
        url_append(out->authority, "[", 1);
    if (!url_append(out->authority, out->host, host_size))
        return 0;
    if (out->numeric && strchr(out->host, ':') && !url_append(out->authority, "]", 1))
        return 0;
    if (number != default_port && (!url_append(out->authority, ":", 1) ||
                                   !url_append(out->authority, out->port, strlen(out->port))))
        return 0;
    query = strchr(end, '?');
    p = strchr(end, '#');
    if (query && p && query > p)
        query = 0;
    if (!p)
        p = end + strlen(end);
    if (!url_copy(path, end, (size_t)((query ? query : p) - end)))
        return 0;
    if (!path[0])
        strcpy(path, "/");
    url_normalize(path);
    strcpy(out->target, path);
    if (query && !url_append(out->target, query, (size_t)(p - query)))
        return 0;
    strcpy(out->canonical, out->tls ? "https://" : "http://");
    return url_append(out->canonical, out->authority, strlen(out->authority)) &&
           url_append(out->canonical, out->target, strlen(out->target));
}
/* Build an absolute candidate before the common authority/path validator publishes its fields. */
FVM_Result fvm_socket_url_resolve(const char *reference, const char *base, FVM_SocketURL *out)
{
    FVM_SocketURL origin;
    char candidate[FVM_DESKTOP_MAX_LOCATION + 1], path[FVM_DESKTOP_MAX_LOCATION + 1];
    const char *colon, *slash, *query;
    size_t size;
    if (!reference || !out || !url_valid(reference))
        return FVM_INVALID_ARGUMENT;
    colon = strchr(reference, ':');
    slash = reference + strcspn(reference, "/?#");
    if (colon && colon < slash)
        return url_absolute(reference, out) ? FVM_OK : FVM_INVALID_ARGUMENT;
    if (!base || !url_valid(base) || !url_absolute(base, &origin))
        return FVM_INVALID_ARGUMENT;
    strcpy(candidate, origin.tls ? "https:" : "http:");
    if (!strncmp(reference, "//", 2))
    {
        if (!url_append(candidate, reference, strlen(reference)))
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        if (!url_append(candidate, "//", 2) ||
            !url_append(candidate, origin.authority, strlen(origin.authority)))
            return FVM_INVALID_ARGUMENT;
        strcpy(path, origin.target);
        query = strchr(path, '?');
        if (query)
            path[query - path] = 0;
        if (!*reference || *reference == '#')
        {
            strcpy(path, origin.target);
        }
        else if (*reference == '?')
        {
            if (!url_append(path, reference, strlen(reference)))
                return FVM_INVALID_ARGUMENT;
        }
        else if (*reference == '/')
        {
            strcpy(path, reference);
        }
        else
        {
            slash = strrchr(path, '/');
            size = slash ? (size_t)(slash - path + 1) : 0;
            path[size] = 0;
            if (!url_append(path, reference, strlen(reference)))
                return FVM_INVALID_ARGUMENT;
        }
        if (!url_append(candidate, path, strlen(path)))
            return FVM_INVALID_ARGUMENT;
    }
    return url_absolute(candidate, out) ? FVM_OK : FVM_INVALID_ARGUMENT;
}
