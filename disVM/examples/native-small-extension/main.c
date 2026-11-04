/* Native-only continuous guard extension; no managed heap or GC permission. */
#include "disvm/small_range.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Main keeps this native record alive through every access. Other writers must
 * use the same canonical domain for this field and its owning reference slot. */
typedef struct Record
{
    unsigned long length; /* Native value; not a serialized Dis64 field. */
} Record;


/* domain/source/output borrow independently live native storage. Capture the
 * source under its guard, extend to the discovered field, then publish output.
 * Return ACQUIRED on success, BUSY/REFUSED with output unchanged otherwise.
 * Always end the original access even after failed extension. No allocation,
 * owner wait, VM callback, retain/root or referent-lifetime proof is supplied. */
static DVM_CellResult read_length(DVM_CellDomain *domain, Record **source, unsigned long *output)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult result;
    Record *captured;
    unsigned long value = 0;
    result = dvm_small_range_access_begin(domain, source, sizeof(*source), source,
        sizeof(*source), output, sizeof(*output), &access);
    if (result != DVM_CELL_ACQUIRED)
    {
        return result;
    }
    memcpy(&captured, source, sizeof(captured));
    if (captured)
    {
        result = dvm_small_range_access_extend(&access, &captured->length, sizeof(value));
        if (result == DVM_CELL_ACQUIRED)
        {
            memcpy(&value, &captured->length, sizeof(value));
        }
    }
    if (result == DVM_CELL_ACQUIRED)
    {
        memcpy(output, &value, sizeof(value));
    }
    if (!dvm_small_range_access_end(&access))
    {
        /* A foreign owner/depth error is fatal here; never pretend it unlocked. */
        fputs("native extension: access retirement failed\n", stderr);
        exit(1);
    }
    return result;
}


/* No arguments. Own one bare native context/domain and a stack record through
 * all operations; retire them only after guard depth is zero. Return0 on success,
 * nonzero on explicit setup/semantic/cleanup failure. No threads or GC are started. */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(void *), NULL);
    DVM_CellDomain *domain;
    Record record = {17};
    Record *source = &record;
    unsigned long output = 99;
    int failed = 0;
    if (!context)
    {
        fputs("native extension: context allocation failed\n", stderr);
        return 1;
    }
    if (!dvm_native_context_bind(context))
    {
        fputs("native extension: context binding failed\n", stderr);
        if (!dvm_native_context_destroy(context))
        {
            fputs("native extension: unbound context retirement failed\n", stderr);
        }
        return 1;
    }
    domain = dvm_cell_domain_create();
    if (!domain || read_length(domain, &source, &output) != DVM_CELL_ACQUIRED || output != 17)
    {
        fputs("native extension: record snapshot failed\n", stderr);
        failed = 1;
    }
    if (!failed)
    {
        source = NULL;
        if (read_length(domain, &source, &output) != DVM_CELL_ACQUIRED || output != 0)
        {
            fputs("native extension: nil snapshot failed\n", stderr);
            failed = 1;
        }
    }
    if (dvm_native_context_object_count(context) ||
        (domain && !dvm_cell_domain_destroy(&domain)) ||
        !dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fputs("native extension: retirement failed\n", stderr);
        return 1;
    }
    if (failed)
    {
        return 1;
    }
    puts("PASS native continuous source/field snapshot");
    return 0;
}
