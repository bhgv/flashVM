/* Copyright (c) 2026 bhgv
 * EN: Loader metadata comes from its open handle, not a later path lookup.
 * RU: Loader metadata берётся из открытого handle, не из нового path lookup.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "filesystem_sys.h"
#include "file_gate.h"
#include "file_slots.h"
#include "loader.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Reassemble borrowed split native bits exactly; no managed conversion.
 * Точно собрать split native bits; без managed conversion. */
static uint64_t number(FVM_FileNumber64 value)
{
    return ((uint64_t)value.high << 32) | (uint64_t)value.low;
}


/* EN: Return owned Dir for an open module FD, or NULL with guest error.
 * Existing loader-native admission pins the slot; its gate serializes only
 * this handle. Full identity/mtime and length come from fstat of that object.
 * Wider-than-retained identity declines cache reuse, not the file load.
 * RU: Вернуть свой Dir для открытого module FD либо NULL с guest error.
 * Loader-native admission держит slot; gate сериализует только этот handle.
 * Identity/mtime/length берутся через fstat объекта. Слишком широкая identity
 * запрещает reuse cache, но не загрузку файла. */
Dir *kdirfstat(int fd)
{
    DVM_FileTicket ticket;
    FVM_FileInfo source;
    FVM_PortFile *file = dvm_runtime_file(fd);
    DVM_FileGate *gate = dvm_runtime_file_gate(fd);
    FVM_Result status;
    Dir *metadata;
    if (!file || !gate)
    {
        kwerrstr("module metadata requires an open file");
        return NULL;
    }
    dvm_file_gate_reserve(gate, &ticket);
    if (!dvm_file_gate_enter_now(gate, &ticket))
    {
        return NULL;
    }
    status = fvm_port_file_info(file, &source);
    if (!dvm_file_gate_leave(gate, &ticket))
    {
        fputs("disvm: loader metadata admission lost\n", stderr);
        abort();
    }
    if (status != FVM_OK || source.directory || number(source.length) >= DVM_MODULE_IMAGE_LIMIT)
    {
        kwerrstr("cannot stat module image or implausible length");
        return NULL;
    }
    metadata = calloc(1, sizeof(*metadata));
    if (!metadata)
    {
        kwerrstr("cannot allocate module metadata");
        return NULL;
    }
    metadata->length = number(source.length);
    metadata->qid.path = number(source.identity);
    metadata->qid.vers = number(source.length);
    metadata->mtime = number(source.modified);
    metadata->dev = source.device.low;
    metadata->type = source.device.high;
    if (source.device.high > USHRT_MAX || sizeof(metadata->mtime) < sizeof(uint64_t))
    {
        metadata->mode |= DVM_MODULE_NO_CACHE;
        metadata->qid.type = DVM_MODULE_UNCACHEABLE_QID;
    }
    return metadata;
}
