/* Independent RFC3986 resolution cases and deliberate strict-HTTP validation policy. */
#include <stdio.h>
#include "socket_url.h"
#include <string.h>
#include <stdlib.h>


static void good(const char *reference, const char *base, const char *canonical)
{
    FVM_SocketURL url;
    if (!(fvm_socket_url_resolve(reference, base, &url) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(url.canonical, canonical)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(url.target[0] == '/'))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Generated hostile references exercise resolver termination/bounds and canonical round trips. */
static void generated(void)
{
    static const char alphabet[] = "abcXYZ01239./?%#:@[];=+-_!\\\r\n\t\377";
    char reference[513];
    FVM_SocketURL first, second;
    unsigned int trial;
    size_t i, size;
    srand(2371);
    for (trial = 0; trial < 10000; ++trial)
    {
        size = (size_t)rand() % sizeof(reference);
        for (i = 0; i < size; ++i)
        {
            reference[i] = alphabet[(size_t)rand() % (sizeof(alphabet) - 1)];
        }
        reference[size] = 0;
        if (fvm_socket_url_resolve(reference, "https://example.com/a/b?old", &first) != FVM_OK)
        {
            continue;
        }
        if (!(strlen(first.canonical) <= FVM_DESKTOP_MAX_LOCATION && first.target[0] == '/'))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strchr(first.canonical, '#') && !strchr(first.canonical, '\r') && !strchr(first.canonical, '\n')))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_socket_url_resolve(first.canonical, 0, &second) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(first.canonical, second.canonical)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(first.target, second.target) && !strcmp(first.host, second.host)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


int main(void)
{
    const char *base = "http://a/b/c/d;p?q";
    const char *refs[] = {
        "g",     "./g",    "g/",        "/g",         "//g",     "?y",      "g?y",        "#s",
        "g#s",   "g?y#s",  ";x",        "g;x",        "g;x?y#s", "",        ".",          "./",
        "..",    "../",    "../g",      "../..",      "../../",  "../../g", "../../../g", "../../../../g",
        "/./g",  "/../g",  "g.",        ".g",         "g..",     "..g",     "./../g",     "./g/.",
        "g/./h", "g/../h", "g;x=1/./y", "g;x=1/../y", "g?y/./x", "g?y/../x"};
    const char *ends[] = {
        "/b/c/g",     "/b/c/g",       "/b/c/g/",      "/g",      "http://g/", "/b/c/d;p?y", "/b/c/g?y",
        "/b/c/d;p?q", "/b/c/g",       "/b/c/g?y",     "/b/c/;x", "/b/c/g;x",  "/b/c/g;x?y", "/b/c/d;p?q",
        "/b/c/",      "/b/c/",        "/b/",          "/b/",     "/b/g",      "/",          "/",
        "/g",         "/g",           "/g",           "/g",      "/g",        "/b/c/g.",    "/b/c/.g",
        "/b/c/g..",   "/b/c/..g",     "/b/g",         "/b/c/g/", "/b/c/g/h",  "/b/c/h",     "/b/c/g;x=1/y",
        "/b/c/y",     "/b/c/g?y/./x", "/b/c/g?y/../x"};
    const char *bad[] = {"ftp://a/x",         "file:///x",       "http:g",        "http://",       "http://u:p@a/x",
                         "http://a:0/",       "http://a:65536/", "http://a:-1/",  "http://a:/",    "http://a:80:81/",
                         "http://a/\r\nX:a",  "http://a/a b",    "http://a/\\x",  "http://a/%",    "http://a/%0",
                         "http://a/%gg",      "http://[::1/x",   "http://[]/",    "http://[abc]/", "http://[::1]:x/",
                         "http://[::1]evil/", "http://a\t/x",    "http://a/\177", "http://a/\377"};
    size_t i;
    char expected[FVM_DESKTOP_MAX_LOCATION + 1], long_url[FVM_DESKTOP_MAX_LOCATION + 2];
    FVM_SocketURL url;
    if (!(sizeof(refs) / sizeof(*refs) == sizeof(ends) / sizeof(*ends)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < sizeof(refs) / sizeof(*refs); ++i)
    {
        if (!strncmp(ends[i], "http:", 5))
        {
            strcpy(expected, ends[i]);
        }
        else
        {
            strcpy(expected, "http://a");
            strcat(expected, ends[i]);
        }
        good(refs[i], base, expected);
    }
    good("HTTP://EXAMPLE.com:00080/a/../b?x=%00#ignored", 0, "http://example.com/b?x=%00");
    good("https://[::1]:443/a", 0, "https://[::1]/a");
    good("//[::1]:8080/a", "https://a/", "https://[::1]:8080/a");
    good("http://a//b///./c/..", 0, "http://a//b///");
    good("http://a/%2e/%2E%2e/x", 0, "http://a/%2e/%2E%2e/x");
    good("http://a/a?old", "embedded://ignored", "http://a/a?old");
    for (i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
    {
        if (!(fvm_socket_url_resolve(bad[i], base, &url) != FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_socket_url_resolve("relative", "not-a-url", &url) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_socket_url_resolve(0, base, &url) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_socket_url_resolve("http://a/", 0, 0) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(long_url, 'a', sizeof(long_url));
    memcpy(long_url, "http://a/", 9);
    long_url[FVM_DESKTOP_MAX_LOCATION] = 0;
    if (!(fvm_socket_url_resolve(long_url, 0, &url) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    long_url[FVM_DESKTOP_MAX_LOCATION] = 'a';
    long_url[FVM_DESKTOP_MAX_LOCATION + 1] = 0;
    if (!(fvm_socket_url_resolve(long_url, 0, &url) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    generated();
    return 0;
}
