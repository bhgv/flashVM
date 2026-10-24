/* Fail every catalog allocation while retaining a valid owner and testing later recovery. */
#include "interface.h"
#include <assert.h>
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
        return NULL;
    {
        void *result = malloc(size);
        if (result)
            ++outstanding;
        return result;
    }
}

/* Cover the catalog owner allocation before any tree is created. */
void *limbo_interface_test_calloc(size_t count, size_t size)
{
    if (calls++ == fail_at)
        return NULL;
    {
        void *result = calloc(count, size);
        if (result)
            ++outstanding;
        return result;
    }
}

/* Reproduce realloc's unchanged-owner contract on a failed array growth. */
void *limbo_interface_test_realloc(void *data, size_t size)
{
    if (calls++ == fail_at)
        return NULL;
    {
        int new_owner = data == NULL;
        void *result = realloc(data, size);
        if (result && new_owner)
            ++outstanding;
        return result;
    }
}

/* Count catalog releases independently of leak-sanitizer availability. */
void limbo_interface_test_free(void *data)
{
    if (data)
    {
        assert(outstanding);
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
        used += (size_t)sprintf(source + used, "v%lu:int;", (unsigned long)i);
    assert(limbo_interface_parse(source, used, &owner, &diagnostic));
    allocations = calls;
    retained = outstanding;
    assert(allocations > 250);
    saved = owner;
    for (i = 0; i < allocations; ++i)
    {
        fail_at = i;
        calls = 0;
        assert(!limbo_interface_parse(source, used, &owner, &diagnostic));
        assert(owner == saved && diagnostic.code == -2 && diagnostic.message[0]);
        assert(outstanding == retained);
        assert(limbo_interface_count(owner) == 250);
        found = SIZE_MAX;
        assert(limbo_interface_find(owner, SIZE_MAX, "v249", &found) && found == 249);
        fail_at = SIZE_MAX;
        calls = 0;
        assert(limbo_interface_parse("M:module{f:fn();};", strlen("M:module{f:fn();};"), &next,
                                     &diagnostic));
        limbo_interface_destroy(next);
        assert(outstanding == retained);
    }
    limbo_interface_destroy(owner);
    assert(outstanding == 0);
    return 0;
}
