/* Copyright (c) 2026 bhgv
 * [GC:] EN: Route ordinary property access to an exact libstyxrpc .var file.
 * RU: Передача обращений к обычному свойству на точный файл .var через libstyxrpc.
 */
#include "flashvm/styxrpc_filesystem.h"
#include "flashvm/gc_roots.h"
#include "flashvm/userdata.h"
#include "managed_values.h"
#include "variable_ref_adapter.h"
#include <stdlib.h>
#include <string.h>

typedef struct IntBinding
{
    StyxRpcVfsFile *file;      /* Held native variable fid, independent of subsequent path lookup. */
    StyxRpcFile local;        /* Held slot file when the publication is local. */
    struct StyxClient *mount; /* Ordinary mount identity associated with the binding. */
    uint64_t node;           /* Captured Qid.path for a transferable module export. */
    char *path;              /* Owned canonical path for a transferable export. */
    int writable;            /* Host-authorized property writes. */
} IntBinding;

typedef struct IntTarget
{
    struct StyxClient *mount;       /* Ordinary mount, also the scope of local aliases. */
    struct StyxRpcModuleSlot *local; /* Selected only for a local module descriptor. */
    const char *path;                /* Borrowed candidate file path for ordinary open. */
    int writable;                   /* Access requested by the original caller. */
    const StyxRpcIntRef *issued;      /* Legacy PID descriptor instead of a path. */
    const StyxRpcModuleRef *module;   /* Paired module descriptor instead of a path. */
} IntTarget;


/* [GC:] EN: Drop the exact variable fid without running transport I/O under reclamation.
 * RU: Снять точный fid переменной без транспортного I/O в сборщике. */
static void release_variable(void *state)
{
    IntBinding *binding = state;
    styxrpc_file_close(&binding->local);
    styxrpc_vfs_close(binding->file, 1);
    free(binding->path);
    free(binding);
}


/* [ASINC] EN: On first use, resolve a returned remote alias on its original mount.
 * RU: При первом обращении разрешить возвращённую ссылку в исходном подключении. */
static int remote_ready(IntBinding *binding)
{
    StyxRpcModuleRef ref = {0};
    if (binding->file)
    {
        return 1;
    }
    if (!binding->path || !binding->node)
    {
        return 0;
    }
    ref.node = binding->node;
    ref.writable = binding->writable;
    strcpy(ref.path, binding->path);
    return styxrpc_module_ref_open_remote(binding->mount, &ref, &binding->file) == STYXRPC_IO_OK;
}


/* [ASINC] EN: Read the owner's current value on every ordinary property access.
 * RU: Читать текущее значение владельца при каждом чтении свойства. */
static FVM_Result read_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    IntBinding *binding = state;
    int32_t value;
    (void)self;
    (void)args;
    if (argc || (binding->local.ops ?
        styxrpc_module_ref_read_local(&binding->local, &value) != STYXRPC_OK :
        !remote_ready(binding) || styxrpc_vfs_int_read(binding->file, &value) != STYXRPC_IO_OK))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC variable read failed");
    }
    fvm_value_number(out, value);
    return FVM_OK;
}


/* EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
int fvm_styxrpc_int_variable_getter(FVM_NativeFunction getter)
{
    return getter == read_variable;
}


/* EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
int fvm_styxrpc_module_ref_from_binding(const void *state, struct StyxClient *mount,
    unsigned lane, StyxRpcModuleRef *out)
{
    const IntBinding *binding = state;
    if (!binding || !binding->node || !binding->path || binding->mount != mount ||
        lane > STYXRPC_MODULE_REF_REVERSE || !out)
    {
        return 0;
    }
    out->node = binding->node;
    out->owner = binding->local.ops ? lane : (1U - lane);
    out->writable = binding->writable;
    strcpy(out->path, binding->path);
    return 1;
}


/* [ASINC] EN: Validate an exact int and issue one replacement on the native variable file.
 * RU: Проверить точный int и выполнить одну запись значения в нативный файл переменной. */
static FVM_Result write_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    IntBinding *binding = state;
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
    if (binding->local.ops)
    {
        status = styxrpc_module_ref_write_local(&binding->local, value) == STYXRPC_OK ?
            STYXRPC_IO_OK : STYXRPC_IO_FAILED;
    }
    else
    {
        status = remote_ready(binding) ? styxrpc_vfs_int_write(binding->file, value) : STYXRPC_IO_NOT_OPEN;
    }
    if (status != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, status == STYXRPC_IO_UNCERTAIN ?
            "Styx RPC variable write outcome is uncertain" : "Styx RPC variable write failed");
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* EN: Open only the requested target, tracking a transferable module Qid when available.
 * RU: Открыть только выбранную цель и сохранить переносимый Qid модуля при наличии. */
static FVM_Result open_target(IntBinding *binding, const IntTarget *target)
{
    const char *path = target->path;
    uint64_t node = 0;
    int opened;
    binding->mount = target->mount;
    binding->writable = target->writable;
    if (target->module)
    {
        const StyxRpcModuleRef *ref = target->module;
        path = ref->path;
        node = ref->node;
        /* EN: Resolve remote fids on access so a batch of aliases can rendezvous first.
         * RU: Открывать удалённые fid при обращении, чтобы сначала передать все ссылки. */
        opened = !target->local ||
            styxrpc_module_ref_open_local(target->local, ref, &binding->local) == STYXRPC_OK;
    }
    else if (target->issued)
    {
        opened = styxrpc_int_ref_open(target->mount, target->issued, &binding->file) == STYXRPC_IO_OK;
    }
    else
    {
        opened = styxrpc_vfs_int_open_identified(target->mount, path, target->writable,
            &binding->file, &node) == STYXRPC_IO_OK;
    }
    if (!opened)
    {
        return FVM_ERROR;
    }
    if (node && styxrpc_module_ref_path(path))
    {
        binding->path = malloc(strlen(path) + 1);
        if (!binding->path)
        {
            return FVM_OUT_OF_MEMORY;
        }
        strcpy(binding->path, path);
        binding->node = node;
    }
    return FVM_OK;
}


/* [GC:] EN: Bind the exact opened variable, selected by path or by its reference.
 * RU: Привязать точную переменную, выбранную путём или ссылкой. */
static FVM_Result bind_variable(FVM_Context *context, FVM_Object *object, const char *name,
    const IntTarget *target)
{
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status;
    IntBinding *binding;
    int own;
    if (!target->mount || (!target->issued && !target->module && (!target->path || !*target->path)) ||
        (target->writable != 0 && target->writable != 1) || !name || !*name ||
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
            status = open_target(binding, target);
            if (status == FVM_OK)
            {
                status = fvm_object_define_accessor_rooted(context, object, name, read_variable, write_variable,
                    binding, value.object);
            }
            if (status != FVM_OK)
            {
                styxrpc_file_close(&binding->local);
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


/* [GC:] EN/RU: See flashvm/styxrpc_filesystem.h. / См. flashvm/styxrpc_filesystem.h. */
FVM_Result fvm_styxrpc_int_variable_bind(FVM_Context *context, FVM_Object *object, const char *name,
    struct StyxClient *session, const char *path, int writable)
{
    IntTarget target = {0};
    target.mount = session;
    target.path = path;
    target.writable = writable;
    return bind_variable(context, object, name, &target);
}


/* [GC:] EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
FVM_Result fvm_styxrpc_int_ref_bind(FVM_Context *context, FVM_Object *object,
    struct StyxClient *session, const StyxRpcIntRef *reference)
{
    IntTarget target = {0};
    if (!reference)
    {
        return FVM_INVALID_ARGUMENT;
    }
    target.mount = session;
    target.writable = reference->writable;
    target.issued = reference;
    return bind_variable(context, object, "value", &target);
}


/* [GC:] EN/RU: See variable_ref_adapter.h. / См. variable_ref_adapter.h. */
FVM_Result fvm_styxrpc_module_ref_bind(FVM_Context *context, FVM_Object *object,
    struct StyxClient *mount, struct StyxRpcModuleSlot *local, const StyxRpcModuleRef *ref)
{
    IntTarget target = {0};
    if (!ref)
    {
        return FVM_INVALID_ARGUMENT;
    }
    target.mount = mount;
    target.local = local;
    target.writable = ref->writable;
    target.module = ref;
    return bind_variable(context, object, "value", &target);
}
