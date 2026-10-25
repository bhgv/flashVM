/* Deterministic allocation failures in the owned tree store, without replacing scanner memory. */
#include <stdio.h>
#include "frontend.h"
#include <stdlib.h>
#include <string.h>

/* SIZE_MAX disables injection; otherwise fail exactly this tree allocation ordinal. */
static size_t fail_at = SIZE_MAX;
static size_t calls;


/* Replace only tree.c malloc calls through a dedicated test object. */
void *limbo_test_malloc(size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    return malloc(size);
}


/* Replace the initial tree owner allocation while preserving real calloc behavior. */
void *limbo_test_calloc(size_t count, size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    return calloc(count, size);
}


/* Preserve the old buffer on failure, matching realloc's ownership contract. */
void *limbo_test_realloc(void *data, size_t size)
{
    if (calls++ == fail_at)
    {
        return NULL;
    }
    return realloc(data, size);
}


/* Sweep every allocation point observed in a parse, retaining an earlier successful owner. */
int main(void)
{
    char source[6006];
    size_t i, allocations;
    LimboTree *owner = NULL, *candidate = NULL, *saved;
    LimboDiagnostic diagnostic;
    memcpy(source, "f(){", 4);
    for (i = 4; i < 6004; i += 2)
    {
        memcpy(source + i, "a;", 2);
    }
    source[6004] = '}';
    if (!(limbo_tree_parse(source, 6005, &owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allocations = calls;
    if (!(allocations > 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = owner;
    for (i = 0; i < allocations; ++i)
    {
        fail_at = i;
        calls = 0;
        if (!(!limbo_tree_parse(source, 6005, &owner, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(owner == saved && diagnostic.code == -2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(limbo_tree_size(owner) == 6005))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_at = SIZE_MAX;
        calls = 0;
        if (!(limbo_tree_parse("implement Recovery;", strlen("implement Recovery;"), &candidate, &diagnostic)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        limbo_tree_destroy(candidate);
        if (!(limbo_check("implement Plain;", strlen("implement Plain;"), NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    limbo_tree_destroy(owner);
    return 0;
}
