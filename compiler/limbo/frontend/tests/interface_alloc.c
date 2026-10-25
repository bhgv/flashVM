/* Fail every catalog allocation while retaining a valid owner and testing later recovery. */
#include <stdio.h>
#include "interface.h"
#include <stdlib.h>
#include <string.h>

/* Fail exactly one owned-catalog allocation ordinal; SIZE_MAX disables injection. */
static size_t fail_at = SIZE_MAX;
static size_t calls;
static size_t outstanding;


/* Replace catalog name/key/scope allocations only, leaving parser/tree memory untouched. */
void *limbo_interface_test_malloc(size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    {
        void *result = malloc(size);
        if (result)
        {
            ++outstanding;
        }
        return result;
    }
}


/* Cover the catalog owner allocation before any tree is created. */
void *limbo_interface_test_calloc(size_t count, size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    {
        void *result = calloc(count, size);
        if (result)
        {
            ++outstanding;
        }
        return result;
    }
}


/* Reproduce realloc's unchanged-owner contract on a failed array growth. */
void *limbo_interface_test_realloc(void *data, size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    {
        int new_owner = data == NULL;
        void *result = realloc(data, size);
        if (result && new_owner)
        {
            ++outstanding;
        }
        return result;
    }
}


/* Count catalog releases independently of leak-sanitizer availability. */
void limbo_interface_test_free(void *data)
{
    if (data)
    {
        if (!(outstanding))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        --outstanding;
    }
    free(data);
}


/* Sweep names, array growth, scope-index and sorted-key allocations with deterministic failures. */
int main(void)
{
    char source[10000];
    size_t i, used = 0, allocations, found, retained;
    LimboInterface *owner = NULL, *saved, *next = NULL;
    LimboDiagnostic diagnostic;
    for (i = 0; i < 250; ++i)
    {
        used += (size_t)sprintf(source + used, "v%lu:int;", (unsigned long)i);
    }
    if (!(limbo_interface_parse(source, used, &owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allocations = calls;
    retained = outstanding;
    if (!(allocations > 250))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = owner;
    for (i = 0; i < allocations; ++i)
    {
        fail_at = i;
        calls = 0;
        if (!(!limbo_interface_parse(source, used, &owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(owner == saved && diagnostic.code == -2 && diagnostic.message[0]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(outstanding == retained))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_interface_count(owner) == 250))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        found = SIZE_MAX;
        if (!(limbo_interface_find(owner, SIZE_MAX, "v249", &found) && found == 249))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_at = SIZE_MAX;
        calls = 0;
        if (!(limbo_interface_parse("M:module{f:fn();};", strlen("M:module{f:fn();};"), &next, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_interface_destroy(next);
        if (!(outstanding == retained))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    limbo_interface_destroy(owner);
    if (!(outstanding == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
