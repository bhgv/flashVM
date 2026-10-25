/* C99 IDE tutorial: source -> FIR -> optimization -> SWO -> SWA -> SWF.
 * All stages call public C APIs in this process. No subprocess or shell is used.
 * See README.md / README.ru.md for build commands, ownership and limitations. */
#include "fir.h"          /* Caller-owned streams, IR snapshots and source/object pipeline. */
#include "swl.h"          /* Independent object/archive/resource linker. */
#include <stdlib.h>       /* Allocate copies that survive callback return. */
#include <string.h>       /* Copy binary output and inspect optimizer phase names. */

#ifdef IDE_WITH_FRONTENDS /* The optional broader parsers only validate syntax. */
#include "as1/frontend/frontend.h"
#include "as2/frontend/frontend.h"
#include "as3/frontend/frontend.h"
#include "limbo/frontend/frontend.h"

#endif


/* One owned binary result. A zero-initialized instance is safe to free. */
typedef struct
{
    unsigned char *data; /* Owned bytes, independent of the writer's borrowed input. */
    size_t size;         /* Exact byte count; SWO/SWA may contain zero bytes. */
} Memory;


/* Copy a once-per-emission writer callback before its borrowed bytes expire. */
static int capture(void *user, const unsigned char *data, size_t n)
{
    Memory *memory = (Memory *)user;
    memory->data = (unsigned char *)malloc(n);
    if (!memory->data)
    {
        return 0; /* Returning failure prevents a claimed successful publication. */
    }
    memcpy(memory->data, data, n);
    memory->size = n;
    return 1;
}


/* Publish the final SWF to the requested path; close even after a short write. */
static int publish(void *user, const unsigned char *data, size_t n)
{
    FILE *out = fopen((const char *)user, "wb");
    int ok;
    if (!out)
    {
        return 0;
    }
    ok = fwrite(data, 1, n, out) == n;
    if (fclose(out))
    {
        ok = 0;
    }
    return ok;
}


/* Inspect borrowed IR only during the callback; never retain its address. */
static int snapshot(void *user, const char *phase, const FirModule *module)
{
    int *count = (int *)user;
    if (!module->count || strcmp(phase, *count % 2 ? "after" : "before"))
    {
        return 0;
    }
    ++*count; /* A real editor could fir_print this module into its own stream. */
    return 1;
}


/* Compile one numeric source module into an owned SWO buffer.
 * Temporary streams belong to this function. The returned object belongs to
 * main; the optimizer callback's counter remains caller-owned throughout. */
static int compile(
    const char *text, FirLanguage language, const char *name, Memory *object, char *error, size_t cap, int *snapshots)
{
    FILE *source = fir_temporary();
    FILE *fir = fir_temporary();
    FILE *out = fir_temporary();
    FirRunOptions options = {1, snapshot, snapshots};
    long length;
    unsigned char *bytes = NULL;
    int ok = 0;

    /* Stage input without exposing console arguments to the library APIs. */
    if (!source || !fir || !out)
    {
        goto done;
    }
    if (fputs(text, source) < 0 || fseek(source, 0, SEEK_SET))
    {
        goto done;
    }

    /* Generate FIR, optimize it and compile an object, not a SWF container. */
    if (!fir_source(source, fir, language, name, error, cap) || fseek(fir, 0, SEEK_SET))
    {
        goto done;
    }
    if (!fir_run(FIR_OBJECT, fir, out, &options, error, cap) || fseek(out, 0, SEEK_END))
    {
        goto done;
    }

    /* Copy the finished object before closing its temporary stream. */
    length = ftell(out);
    if (length < 0 || fseek(out, 0, SEEK_SET))
    {
        goto done;
    }
    bytes = (unsigned char *)malloc((size_t)length);
    if (!bytes || fread(bytes, 1, (size_t)length, out) != (size_t)length)
    {
        goto done;
    }
    object->data = bytes;
    object->size = (size_t)length;
    bytes = NULL; /* Ownership moved to the caller only after a complete read. */
    ok = 1;


done:
    free(bytes); /* Free only bytes not transferred to the caller. */
    if (source)
    {
        fclose(source);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (out)
    {
        fclose(out);
    }
    return ok;
}


/* Build two numeric modules and one visible resource, releasing every owner. */
int main(int argc, char **argv)
{
    Memory first = {0}, second = {0}, library = {0};
    SwlObject objects[2]; /* Borrow first/second bytes only during swl_archive. */
    SwlContext *linker = NULL;
    char error[512] = "IDE I/O failure";
    int ok = 0, snapshots = 0;
    if (argc != 2)
    {
        return 2; /* One required output path; API stages themselves have no argv. */
    }

#ifdef IDE_WITH_FRONTENDS
    /* Broad syntax acceptance is separate from the limited numeric lowering. */
    if (!as1_check("var x = 1;", 10, NULL) || !as2_check("var x = 1;", 10, NULL) ||
        !as3_check("var x = 1;", 10, NULL) || !limbo_check("implement A;", 12, NULL))
    {
        goto done;
    }
#endif

    /* Unique module names avoid duplicate object symbols. Expected traces: 42, 36. */
    if (!compile("var x:Number = 6; trace(x * 7);", FIR_AS2, "first", &first, error, sizeof(error), &snapshots) ||
        !compile("implement V; V: module { value: fn(): int; }; "
                 "value(): int { x: int = 42; return x - 6; }",
                 FIR_LIMBO,
                 "second",
                 &second,
                 error,
                 sizeof(error),
                 &snapshots))
    {
        goto done;
    }

    /* Package both whole objects into an independent library byte buffer. */
    objects[0].data = first.data;
    objects[0].size = first.size;
    objects[1].data = second.data;
    objects[1].size = second.size;
    if (!swl_archive(objects, 2, capture, &library, error, sizeof(error)))
    {
        goto done;
    }

    /* The linker copies accepted inputs, owns resource definitions and preserves order. */
    linker = swl_create(NULL);
    if (!linker || !swl_add_archive(linker, library.data, library.size, error, sizeof(error)) ||
        !swl_add_rect(linker, "panel", 120, 80, 0x20a060ff, error, sizeof(error)) ||
        !swl_place(linker, "panel", 20, 30, 1, error, sizeof(error)) || snapshots != 4)
    {
        goto done;
    }
    ok = swl_emit(linker, publish, argv[1], error, sizeof(error));


done:
    if (!ok)
    {
        fprintf(stderr, "%s\n", error);
    }
    swl_destroy(linker); /* NULL is safe; destroy before releasing our independent buffers. */
    free(first.data);
    free(second.data);
    free(library.data);
    return ok ? 0 : 1;
}
