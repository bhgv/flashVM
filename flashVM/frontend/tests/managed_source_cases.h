/* Copyright (c) 2026 bhgv
 * EN: Real player exercises explicit managed acquisition, provenance and closed fallback boundaries.
 * RU: Настоящий player проверяет managed acquisition, provenance и закрытый fallback.
 */
#include "../common/managed_source.h"
#include "managed_movies.h"


/* EN/RU: One synchronous fixture run; no VM/native pointer is serialized or retained by fetch.
 * Один synchronous fixture run; fetch не сериализует/удерживает VM/native pointers. */
typedef struct ManagedCase
{
    unsigned int calls; /* All actual bridge acquisitions. */
    unsigned int code; /* Code-purpose acquisitions. */
    unsigned int data; /* Data-purpose acquisitions. */
    int failure; /* Selected refusal/malformed-provider scenario. */
    int replace_root; /* Startup movie requests deferred whole-root replacement. */
    const char *initial; /* Borrowed startup request text through the run. */
    const char *root_base; /* Expected copied provenance of the root movie. */
} ManagedCase;


/* EN/RU: Fail any assertion independently of optimized build configuration.
 * Явно завершить нарушение независимо от optimized build configuration. */
static void managed_check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "managed source: %s\n", message);
        exit(EXIT_FAILURE);
    }
}


/* EN/RU: Bridge receives explicit purpose and raw VFS names, serves binary chunks/variable text.
 * Inputs borrowed through return, output canonical VFS origin is bounded; failures never open files.
 * Bridge получает purpose/raw names, выдаёт chunks; inputs borrowed, local files не открываются. */
static FVM_Result managed_fetch(unsigned int purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *sink, char *resolved, size_t capacity, void *userdata)
{
    ManagedCase *test = (ManagedCase *)userdata;
    const void *bytes = 0;
    size_t size = 0, offset, chunk;
    const char *origin = "/vfs/root.swf";
    ++test->calls;
    if (test->failure == 1)
    {
        return FVM_NOT_FOUND;
    }
    if (!base)
    {
        managed_check(purpose == FVM_DESKTOP_FETCH_CODE && !strcmp(reference, test->initial), "startup purpose/reference");
        bytes = test->replace_root ? parent_movie : managed_root_movie;
        size = test->replace_root ? sizeof(parent_movie) : sizeof(managed_root_movie);
        ++test->code;
    }
    else if (!strcmp(reference, "next.swf"))
    {
        managed_check(test->replace_root && purpose == FVM_DESKTOP_FETCH_CODE &&
            !strcmp(base, "/vfs/root.swf"), "root replacement code/base");
        bytes = managed_root_movie;
        size = sizeof(managed_root_movie);
        origin = "/vfs/replaced/root.swf";
        ++test->code;
    }
    else if (!strcmp(reference, "nested/child.swf"))
    {
        managed_check(purpose == FVM_DESKTOP_FETCH_CODE && !strcmp(base, test->root_base), "nested code/base");
        bytes = managed_child_movie;
        size = sizeof(managed_child_movie);
        origin = "/vfs/nested/child.swf";
        ++test->code;
    }
    else if (!strcmp(reference, "root-vars.swf"))
    {
        managed_check(purpose == FVM_DESKTOP_FETCH_DATA && !strcmp(base, test->root_base), "root data/base");
        bytes = "greeting=hello+from+root";
        size = strlen((const char *)bytes);
        origin = "/vfs/root-vars.swf";
        ++test->data;
    }
    else if (!strcmp(reference, "child-vars.txt"))
    {
        managed_check(purpose == FVM_DESKTOP_FETCH_DATA && !strcmp(base, "/vfs/nested/child.swf"), "child data/base");
        bytes = "answer=child+origin+works";
        size = strlen((const char *)bytes);
        origin = "/vfs/nested/child-vars.txt";
        ++test->data;
    }
    else
    {
        managed_check(0, "unexpected resource request");
    }
    managed_check(capacity > strlen(origin), "canonical origin capacity");
    strcpy(resolved, origin);
    if (test->failure == 2 || test->failure == 4)
    {
        FVM_Result status = write(embedded_movie, sizeof(embedded_movie), sink);
        managed_check(status == FVM_OK, "complete staged movie");
        if (test->failure == 4)
        {
            memset(resolved, 'x', capacity);
            return FVM_OK;
        }
        return FVM_ERROR;
    }
    if (test->failure == 3)
    {
        managed_check(write(embedded_movie, (size_t)FVM_SWF_DEFAULT_LIMIT + 1, sink) != FVM_OK, "sink limit");
        return FVM_OK;
    }
    for (offset = 0; offset < size; offset += chunk)
    {
        FVM_Result status;
        chunk = size-offset > 3 ? 3 : size-offset;
        status = write((const unsigned char *)bytes+offset, chunk, sink);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    return FVM_OK;
}


/* EN/RU: Test real nested SWF execution in both caller-selected VM modes and native file denial.
 * options borrowed; all temporary native files and policies are retired before return.
 * Проверить nested SWF и отказ fallback; options borrowed, временные files/policies освобождены. */
static void managed_source_cases(const FVM_DesktopOptions *options)
{
    FVM_DesktopOptions run = *options;
    FVM_DesktopManagedLoader loader;
    FVM_DesktopSource source;
    ManagedCase test;
    const char *error;
    const char *filename = options->mode == FVM_EXECUTION_JIT ? "managed-source-jit.data" : "managed-source-interpreter.data";
    FILE *file = fopen(filename, "wb");
    unsigned int kind;
    int failure;
    managed_check(file && fwrite(embedded_movie, 1, sizeof(embedded_movie), file) == sizeof(embedded_movie) &&
        fclose(file) == 0, "create valid native fallback fixture");
    managed_check(fvm_desktop_managed_loader_init(&loader) && !fvm_desktop_managed_loader_init(0), "managed defaults");
    loader.fetch = managed_fetch;
    loader.max_loads = 6;
    loader.userdata = &test;
    run.frames = 4;
    for (kind = FVM_DESKTOP_SOURCE_MEMORY; kind <= FVM_DESKTOP_SOURCE_URL; ++kind)
    {
        memset(&test, 0, sizeof(test));
        source.kind = kind;
        source.bytes = kind == FVM_DESKTOP_SOURCE_MEMORY ? managed_root_movie : 0;
        source.size = kind == FVM_DESKTOP_SOURCE_MEMORY ? sizeof(managed_root_movie) : 0;
        source.location = kind == FVM_DESKTOP_SOURCE_MEMORY ? "granted/root.swf" :
            kind == FVM_DESKTOP_SOURCE_FILE ? filename : "https://bridge.invalid/root.swf";
        test.initial = source.location;
        test.root_base = kind == FVM_DESKTOP_SOURCE_MEMORY ? source.location : "/vfs/root.swf";
        managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == 0 && !error &&
            test.code == (kind == FVM_DESKTOP_SOURCE_MEMORY ? 1U : 2U) && test.data == 2 &&
            test.calls == test.code+test.data, "managed nested code/data execution");
    }
    memset(&test, 0, sizeof(test));
    test.replace_root = 1;
    test.initial = filename;
    test.root_base = "/vfs/replaced/root.swf";
    source.kind = FVM_DESKTOP_SOURCE_FILE;
    source.bytes = 0;
    source.size = 0;
    source.location = filename;
    managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == 0 && !error &&
        test.code == 3 && test.data == 2 && test.calls == 5, "root replacement lost managed bridge/provenance");
    memset(&test, 0, sizeof(test));
    test.replace_root = 1;
    test.initial = filename;
    loader.max_loads = 1;
    managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == FVM_DESKTOP_EXIT_PLAYBACK &&
        error && test.calls == 1, "replacement escaped shared acquisition budget");
    loader.max_loads = 6;
    source.kind = FVM_DESKTOP_SOURCE_FILE;
    source.bytes = 0;
    source.size = 0;
    source.location = filename;
    for (failure = 1; failure <= 4; ++failure)
    {
        memset(&test, 0, sizeof(test));
        test.initial = filename;
        test.failure = failure;
        managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == FVM_DESKTOP_EXIT_PLAYBACK &&
            error && test.calls == 1, "failure fell back to native file or committed partial movie");
    }
    memset(&test, 0, sizeof(test));
    loader.fetch = 0;
    managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == FVM_DESKTOP_EXIT_ARGUMENT &&
        !test.calls, "missing bridge silently enabled native I/O");
    loader.fetch = managed_fetch;
    loader.struct_size = 0;
    managed_check(fvm_desktop_run_source_managed(&run, &source, &loader, &error) == FVM_DESKTOP_EXIT_ARGUMENT &&
        !test.calls, "invalid managed descriptor admitted");
    managed_check(remove(filename) == 0, "native fixture cleanup");
    puts("PASS managed startup root replacement nested SWF data budget and closed native fallback");
}
