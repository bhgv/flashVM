/* Copyright (c) 2026 bhgv
 * EN: Limbo-created atomic integer cell exposed as one exact VFS file.
 * RU: Атомарная целочисленная ячейка Limbo в виде точного файла VFS.
 */
#include "cap_internal.h"
#include "provider_internal.h"
#include "cancel.h"
#include "../../native/terminal/variable_file.h"

typedef struct VariableCell
{
    WalkEntity identity; /* Native identity for exact captures and replacement. */
    int32_t value; /* Atomic signed value. */
    DVM_Changes *journal; /* Borrowed from the owning provider. */
} VariableCell;


typedef struct VariableCursor
{
    StyxIntVariable io; /* Owner pinned by the opened provider. */
    size_t offset; /* Per-open position. */
} VariableCursor;


/* EN/RU: Describe the fixed-size root file.
 * Описать корневой файл фиксированного размера. */
static FVM_Result metadata(void *state, const char *path, FVM_FileInfo *out)
{
    const VariableCell *cell = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "int");
    out->identity.high = cell->identity.object[2];
    out->identity.low = cell->identity.object[3];
    out->mode = 0666;
    out->length.low = STYX_VARIABLE_INT_SIZE;
    return FVM_OK;
}


/* EN/RU: Return metadata of the opened root.
 * Вернуть метаданные открытого корня. */
static FVM_Result stat_open(void *state, FVM_FileInfo *out)
{
    return metadata(((VariableCursor *)state)->io.state, ".", out);
}


/* EN/RU: Snapshot the native value with no managed references.
 * Прочитать нативное значение без управляемых ссылок. */
static int load_value(void *state, int32_t *value)
{
    *value = __atomic_load_n(&((VariableCell *)state)->value, __ATOMIC_ACQUIRE);
    return 1;
}


/* EN/RU: Commit exactly one native scalar replacement.
 * Выполнить одну замену нативного скаляра. */
static int store_value(void *state, int32_t value)
{
    VariableCell *cell = state;
    int32_t previous = __atomic_exchange_n(&cell->value, value, __ATOMIC_ACQ_REL);
    if (previous != value)
    {
        changespost(cell->journal, DVM_CHANGE_CONTENT, 0, 0);
    }
    return 1;
}


/* EN/RU: Read one whole value through the current cursor.
 * Прочитать целое значение через текущую позицию. */
static void read_cell(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    VariableCursor *cursor = state;
    *out = (DVM_IoResult){.status = FVM_ERROR, .flags = DVM_IO_PREFIX, .stop = DVM_IO_DENIED};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (styxintvariableread(&cursor->io, cursor->offset, bytes, &size))
    {
        cursor->offset += size;
        out->status = FVM_OK;
        out->count = size;
        out->stop = size ? DVM_IO_NONE : DVM_IO_EOF;
    }
}


/* EN/RU: Replace the cell only after a complete admitted write.
 * Заменить ячейку только после допустимой полной записи. */
static void write_cell(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    VariableCursor *cursor = state;
    *out = (DVM_IoResult){.status = FVM_ERROR, .flags = DVM_IO_PREFIX, .stop = DVM_IO_DENIED};
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (styxintvariablewrite(&cursor->io, cursor->offset, bytes, size))
    {
        cursor->offset += size;
        out->status = FVM_OK;
        out->count = size;
        out->stop = DVM_IO_NONE;
    }
}


/* EN/RU: Move within the fixed-size file, including its EOF position.
 * Переместить позицию в пределах файла, включая EOF. */
static FVM_Result seek_cell(void *state, long offset, int origin, long *position)
{
    VariableCursor *cursor = state;
    long base = origin == FVM_FILE_SEEK_START ? 0 : origin == FVM_FILE_SEEK_CURRENT ? (long)cursor->offset :
        origin == FVM_FILE_SEEK_END ? (long)STYX_VARIABLE_INT_SIZE : -1;
    if (base < 0 || offset < -base || offset > (long)STYX_VARIABLE_INT_SIZE - base ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    *position = base + offset;
    cursor->offset = (size_t)*position;
    return FVM_OK;
}

static const DVM_VfsOps stream = {.info = stat_open, .read_chunk = read_cell,
    .write_chunk = write_cell, .seek = seek_cell, .close = free}; /* Ordinary exact-file I/O. */


/* EN/RU: Make a mode-restricted cursor for the live integer.
 * Создать курсор с указанными правами для живого целого. */
static FVM_Result open_cell(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    int reading = !strcmp(mode, "r") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    int writing = !strcmp(mode, "wo") || !strcmp(mode, "rw") || !strcmp(mode, "rwo");
    if ((!reading && !writing) || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    VariableCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->io = (StyxIntVariable){state, reading ? load_value : NULL, writing ? store_value : NULL};
    out->state = cursor;
    out->ops = &stream;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_SEEK | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}

static const DVM_ProviderOps factory = {.info = metadata, .open = open_cell,
    .destroy = free, .flags = DVM_PROVIDER_STABLE_BIND}; /* Exact cell capability. */


/* EN: Recognize a native integer cell without opening it.
 * RU: Распознать нативную целочисленную ячейку без открытия. */
int dvm_cap_is_integer(const DVM_CapState *state)
{
    return state && state->root && state->root->ops == &factory;
}


/* EN/RU: Create a Limbo-owned root with initial signed-32-bit value.
 * Создать корень Limbo с начальным знаковым 32-битным значением. */
void Vfsexport_intvar(void *frame)
{
    F_Vfsexport_intvar call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_intvar, ret), sizeof(*call.ret));
    if (call.initial < INT32_MIN || call.initial > INT32_MAX)
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state)
    {
        VariableCell *cell = calloc(1, sizeof(*cell));
        if (cell && walk_entity(&cell->identity))
        {
            cell->value = call.initial;
            state->root = dvm_provider_create_native(&factory, cell, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
            if (state->root)
            {
                cell->journal = dvm_provider_changes(state->root);
            }
        }
        if (!state->root)
        {
            free(cell);
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Return an exact typed snapshot or status zero for an unrelated root.
 * Вернуть типизированный снимок либо нулевой статус для другого корня. */
void Vfsexport_intget(void *frame)
{
    F_Vfsexport_intget call;
    WORD result[2] = {0, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_intget, ret), sizeof(result));
    DVM_CapState *state = dvm_cap_get(call.root, DVM_CAP_ROOT);
    int32_t value;
    if (state && state->root && state->root->ops == &factory && load_value(state->root->state, &value))
    {
        result[0] = 1;
        result[1] = value;
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Set the typed value through the exact original capability.
 * Записать типизированное значение через точную исходную ссылку. */
void Vfsexport_intset(void *frame)
{
    F_Vfsexport_intset call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_intset, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.root, DVM_CAP_ROOT);
    WORD result = state && state->root && state->root->ops == &factory && call.value >= INT32_MIN &&
        call.value <= INT32_MAX && store_value(state->root->state, call.value);
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
