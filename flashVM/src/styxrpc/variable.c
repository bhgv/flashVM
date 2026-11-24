/* Copyright (c) 2026 bhgv
 * [GC:] EN: Route ordinary property access to an exact libstyxrpc .var file.
 * RU: Передача обращений к обычному свойству на точный файл .var через libstyxrpc.
 */
#include "flashvm/styxrpc_filesystem.h"
#include "flashvm/gc_roots.h"
#include "flashvm/userdata.h"
#include "managed_values.h"
#include "styxrpc/vfs_client.h"
#include <stdlib.h>

typedef struct IntBinding
{
    StyxRpcVfsFile *file; /* Held native variable fid, independent of subsequent path lookup. */
    int writable;     /* Host-authorized property writes. */
} IntBinding;


/* [GC:] EN: Drop the exact variable fid without running transport I/O under reclamation.
 * RU: Снять точный fid переменной без транспортного I/O в сборщике. */
static void release_variable(void *state)
{
    IntBinding *binding = state;
    styxrpc_vfs_close(binding->file, 1);
    free(binding);
}


/* EN: Read the owner's current value on every ordinary property access.
 * RU: Читать текущее значение владельца при каждом чтении свойства. */
static FVM_Result read_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const IntBinding *binding = state;
    int32_t value;
    (void)self;
    (void)args;
    if (argc || styxrpc_vfs_int_read(binding->file, &value) != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC variable read failed");
    }
    fvm_value_number(out, value);
    return FVM_OK;
}


/* EN: Validate an exact int and issue one replacement on the native variable file.
 * RU: Проверить точный int и выполнить одну запись значения в нативный файл переменной. */
static FVM_Result write_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const IntBinding *binding = state;
    StyxRpcIoOutcome status;
    int32_t value;
    (void)self;
    if (!binding->writable)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC variable is read only");
    }
    if (argc != 1 || args[0].type != FVM_VALUE_NUMBER ||
        !(args[0].number >= INT32_MIN && args[0].number <= INT32_MAX))
    {
        return FVM_INVALID_ARGUMENT;
    }
    value = (int32_t)args[0].number;
    if ((double)value != args[0].number)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_vfs_int_write(binding->file, value);
    if (status != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, status == STYXRPC_IO_UNCERTAIN ?
            "Styx RPC variable write outcome is uncertain" : "Styx RPC variable write failed");
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_filesystem.h. / См. flashvm/styxrpc_filesystem.h. */
FVM_Result fvm_styxrpc_int_variable_bind(FVM_Context *context, FVM_Object *object, const char *name,
    struct StyxClient *session, const char *path, int writable)
{
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status;
    IntBinding *binding;
    int own;
    if (!session || !path || !*path || (writable != 0 && writable != 1) || !name || !*name ||
        fvm_object_has_own(context, object, name, &own) != FVM_OK || own)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    status = fvm_gc_roots_add(roots, &value);
    if (status == FVM_OK)
    {
        status = fvm_object_create(context, &value.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, &value);
    }
    if (status == FVM_OK)
    {
        binding = calloc(1, sizeof(*binding));
        status = binding ? fvm_userdata_attach(context, value.object, binding, release_variable) : FVM_OUT_OF_MEMORY;
        if (status == FVM_OK)
        {
            binding->writable = writable;
            status = styxrpc_vfs_int_open(session, path, writable, &binding->file) == STYXRPC_IO_OK ?
                fvm_object_define_accessor_rooted(context, object, name, read_variable, write_variable, binding, value.object) :
                FVM_ERROR;
            if (status != FVM_OK)
            {
                styxrpc_vfs_close(binding->file, 0);
                binding->file = NULL;
            }
        }
        else
        {
            free(binding);
        }
    }
    fvm_gc_roots_end(roots);
    return status;
}
