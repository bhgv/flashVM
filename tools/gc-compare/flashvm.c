/* Measure the real FlashVM collector through public APIs; no substitute allocator. */
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Keep the benchmark fail-closed even in Release builds with NDEBUG. */
#define REQUIRE(call)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(call))                                                                                                   \
        {                                                                                                              \
            fprintf(stderr, "failed line %d: %s\n", __LINE__, #call);                                                  \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* CPU time is not wall-clock tail latency; the runner labels it accordingly. */
static double milliseconds(clock_t start, clock_t end)
{
    return 1000.0 * (double)(end - start) / (double)CLOCKS_PER_SEC;
}


/* Build a dead/live ring or dead objects, then measure complete collections. */
int main(int argc, char **argv)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object **objects = 0;
    FVM_Value value;
    FVM_GCStringStats strings_before;
    FVM_GCStringStats strings_after;
    unsigned long count;
    unsigned long index;
    unsigned long baseline;
    unsigned long before;
    unsigned long after;
    unsigned long reclaimed;
    clock_t start;
    clock_t allocated;
    clock_t kept;
    clock_t collected;
    double keep_ms = 0;
    int is_string;
    int rooted;
    int ring;
    REQUIRE(argc == 3);
    count = strtoul(argv[2], 0, 10);
    REQUIRE(count && count <= 1000000UL);
    is_string = !strcmp(argv[1], "strings_arena") || !strcmp(argv[1], "strings_traced");
    rooted = !strcmp(argv[1], "cycle_rooted");
    ring = rooted || !strcmp(argv[1], "cycle_dead");
    REQUIRE(is_string || ring || !strcmp(argv[1], "objects_dead"));
    REQUIRE(fvm_config_init(&config) == FVM_OK);
    config.memory_limit_bytes = 1024UL * 1024UL * 1024UL;
    REQUIRE(fvm_runtime_create(&config, &runtime) == FVM_OK);
    REQUIRE(fvm_context_create(runtime, &context) == FVM_OK);
    REQUIRE(fvm_gc_mode(context, FVM_GC_SUSPENDED) == FVM_OK);
    REQUIRE(fvm_gc_object_count(context, &baseline) == FVM_OK);
    if (!strcmp(argv[1], "strings_traced"))
    {
        REQUIRE(fvm_gc_string_mode(context, FVM_GC_STRINGS_TRACED) == FVM_OK);
    }
    fvm_value_undefined(&value);
    start = clock();
    if (is_string)
    {
        for (index = 0; index < count; index++)
        {
            unsigned short text[32];
            char ascii[32];
            size_t length;
            size_t character;
            sprintf(ascii, "text-%lu-suffix", index);
            length = strlen(ascii);
            for (character = 0; character < length; character++)
            {
                text[character] = (unsigned char)ascii[character];
            }
            REQUIRE(fvm_value_string_utf16(context, text, length, &value) == FVM_OK);
        }
    }
    else
    {
        objects = (FVM_Object **)calloc(count, sizeof(*objects));
        REQUIRE(objects);
        for (index = 0; index < count; index++)
        {
            REQUIRE(fvm_object_create(context, &objects[index]) == FVM_OK);
        }
        if (ring)
        {
            value.type = FVM_VALUE_OBJECT;
            for (index = 0; index < count; index++)
            {
                value.object = objects[(index + 1) % count];
                REQUIRE(fvm_object_set(context, objects[index], "next", &value) == FVM_OK);
            }
        }
    }
    fvm_value_undefined(&value);
    REQUIRE(fvm_gc_object_count(context, &before) == FVM_OK);
    REQUIRE(fvm_gc_string_stats(context, &strings_before) == FVM_OK);
    allocated = clock();
    if (rooted)
    {
        REQUIRE(fvm_gc_pin(context, objects[0]) == FVM_OK);
        REQUIRE(fvm_gc_collect(context, &reclaimed) == FVM_OK && reclaimed == 0);
        kept = clock();
        keep_ms = milliseconds(allocated, kept);
        REQUIRE(fvm_gc_unpin(context, objects[0]) == FVM_OK);
    }
    else
    {
        kept = allocated;
    }
    REQUIRE(fvm_gc_collect(context, &reclaimed) == FVM_OK);
    collected = clock();
    REQUIRE(fvm_gc_object_count(context, &after) == FVM_OK);
    REQUIRE(fvm_gc_string_stats(context, &strings_after) == FVM_OK);
    REQUIRE(after == baseline && (is_string || reclaimed == count));
    if (!strcmp(argv[1], "strings_arena"))
    {
        REQUIRE(strings_after.buffers == strings_before.buffers);
    }
    if (!strcmp(argv[1], "strings_traced"))
    {
        REQUIRE(strings_after.buffers == 0);
    }
    printf("{\"case\":\"%s\",\"count\":%lu,\"allocation_cpu_ms\":%.6f,\"keep_collect_cpu_ms\":%.6f,"
           "\"drop_collect_cpu_ms\":%.6f,\"objects_before\":%lu,\"objects_after\":%lu,"
           "\"strings_before_bytes\":%lu,\"strings_after_bytes\":%lu}\n",
           argv[1],
           count,
           milliseconds(start, allocated),
           keep_ms,
           milliseconds(kept, collected),
           before,
           after,
           (unsigned long)strings_before.allocated_bytes,
           (unsigned long)strings_after.allocated_bytes);
    free(objects);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
