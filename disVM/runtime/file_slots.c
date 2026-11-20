/* Copyright (c) 2026 bhgv
 * EN: Synchronized native descriptor reservation, publication and detachment.
 * RU: Синхронные резервация, публикация и отделение нативных дескрипторов.
 */
#include "file_slots.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DVM_FileSlot slots[DVM_FILE_SLOT_LIMIT]; /* Registry mutex protects slots; 0..2 are host stdio. */
static unsigned references[DVM_FILE_SLOT_LIMIT - DVM_LAUNCH_SLOT_FIRST]; /* Shared launch-slot owners under registry mutex. */


/* EN: Return whether descriptor is in 3..31 without accessing registry storage.
 * RU: Вернуть принадлежность descriptor диапазону 3..31 без доступа к реестру. */
static int valid(int descriptor)
{
    return descriptor >= 3 && descriptor < DVM_FILE_SLOT_LIMIT;
}


/* EN/RU: Reserve gate/job in [first, limit), skipping excluded ordinary slots.
 * Зарезервировать gate/job в [first, limit), исключая занятые обычные номера. */
static int reserve(DVM_FileGate *gate, DVM_CloseJob *job, int first, int limit, uint32_t excluded)
{
    if (!gate || !job)
    {
        fprintf(stderr, "disvm: invalid native slot reservation\n");
        abort();
    }
    int index;
    fvm_port_registry_lock();
    for (index = first; index < limit; index++)
    {
        if (!slots[index].gate && (index >= DVM_LAUNCH_SLOT_FIRST || !(excluded & (UINT32_C(1) << index))))
        {
            slots[index].gate = gate;
            slots[index].job = job;
            if (index >= DVM_LAUNCH_SLOT_FIRST)
            {
                references[index - DVM_LAUNCH_SLOT_FIRST] = 1;
            }
            break;
        }
    }
    fvm_port_registry_unlock();
    return index == limit ? -1 : index;
}


/* EN/RU: Reserve an ordinary slot with no excluded guest numbers.
 * Зарезервировать обычный слот без исключений номеров гостя. */
int dvm_file_slots_reserve(DVM_FileGate *gate, DVM_CloseJob *job)
{
    return dvm_file_slots_reserve_masked(gate, job, 0);
}


/* EN/RU: See file_slots.h; excluded applies only to ordinary slots.
 * См. file_slots.h; исключения применяются только к обычным слотам. */
int dvm_file_slots_reserve_masked(DVM_FileGate *gate, DVM_CloseJob *job, uint32_t excluded)
{
    return reserve(gate, job, 3, DVM_LAUNCH_SLOT_FIRST, excluded);
}


/* EN/RU: See file_slots.h; this range is separate from ordinary open and loader slots.
 * См. file_slots.h; этот диапазон отдельный от обычных открытий и загрузчика. */
int dvm_file_slots_reserve_launch(DVM_FileGate *gate, DVM_CloseJob *job)
{
    return reserve(gate, job, DVM_LAUNCH_SLOT_FIRST, DVM_FILE_SLOT_LIMIT, 0);
}


/* EN/RU: See file_slots.h; the caller's existing pin prevents slot reuse.
 * См. file_slots.h; удерживаемая ссылка исключает повторное применение слота. */
int dvm_file_slots_retain_launch(int descriptor)
{
    if (descriptor < DVM_LAUNCH_SLOT_FIRST || !valid(descriptor))
    {
        return 0;
    }
    fvm_port_registry_lock();
    unsigned *count = &references[descriptor - DVM_LAUNCH_SLOT_FIRST];
    int retained = slots[descriptor].resource.ops && *count && *count != (unsigned)-1;
    if (retained)
    {
        ++*count;
    }
    fvm_port_registry_unlock();
    return retained;
}


/* EN/RU: See file_slots.h; use the reserved close job after the registry detach.
 * См. file_slots.h; использовать резерв закрытия после снятия с реестра. */
void dvm_file_slots_drop_launch(int descriptor)
{
    DVM_FileSlot retired = {0};
    if (descriptor < DVM_LAUNCH_SLOT_FIRST || !valid(descriptor))
    {
        abort();
    }
    fvm_port_registry_lock();
    unsigned *count = &references[descriptor - DVM_LAUNCH_SLOT_FIRST];
    if (!*count)
    {
        abort();
    }
    if (!--*count)
    {
        retired = slots[descriptor];
        memset(&slots[descriptor], 0, sizeof(slots[descriptor]));
    }
    fvm_port_registry_unlock();
    if (retired.gate && !dvm_file_gate_destroy(retired.gate))
    {
        abort();
    }
    if (retired.resource.ops)
    {
        dvm_close_submit_resource(retired.job, &retired.resource);
    }
    else
    {
        dvm_close_discard(retired.job);
    }
}


/* EN: Publish exactly one handle under the reservation's registry mutex.
 * descriptor belongs to this caller; file/directory transfer on success.
 * RU: Опубликовать ровно один handle под мьютексом резервации.
 * descriptor принадлежит вызывающему; владение file/directory передаётся. */
void dvm_file_slots_publish(int descriptor, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    DVM_VfsHandle resource = {0};
    if ((!file && !directory) || !dvm_vfs_adopt(&resource, file, directory))
    {
        abort();
    }
    dvm_file_slots_publish_resource(descriptor, &resource);
}


/* EN/RU: See file_slots.h; source is exclusively owned before slot publication.
 * См. file_slots.h; source эксклюзивно принадлежит caller до публикации. */
void dvm_file_slots_publish_resource(int descriptor, DVM_VfsHandle *resource)
{
    fvm_port_registry_lock();
    if (!valid(descriptor) || !slots[descriptor].gate || slots[descriptor].resource.ops ||
        !resource || !resource->ops || !dvm_vfs_move(&slots[descriptor].resource, resource))
    {
        fvm_port_registry_unlock();
        fprintf(stderr, "disvm: invalid provider slot publication\n");
        abort();
    }
    fvm_port_registry_unlock();
}


/* EN: Copy the exclusive descriptor into output and clear its entire registry slot.
 * published requires a ready handle. No lifecycle work occurs while locked.
 * RU: Скопировать личный descriptor в output и очистить весь слот реестра.
 * published требует готовый handle. Работа с жизнью ресурсов идёт вне мьютекса. */
int dvm_file_slots_take(int descriptor, int published, DVM_FileSlot *output)
{
    fvm_port_registry_lock();
    if (!valid(descriptor) || descriptor >= DVM_LAUNCH_SLOT_FIRST || !slots[descriptor].gate ||
        (published && !slots[descriptor].resource.ops))
    {
        fvm_port_registry_unlock();
        return 0;
    }
    *output = slots[descriptor];
    memset(&slots[descriptor], 0, sizeof(slots[descriptor]));
    fvm_port_registry_unlock();
    return 1;
}


/* EN: Borrow descriptor's file under the mutex; rooted FD/loader pins lifetime.
 * Return NULL for stdio, reserved or non-file slots. No ownership transfer.
 * RU: Заимствовать файл descriptor под мьютексом; корень FD/загрузчик держит жизнь.
 * Для stdio, резерва или не-файла вернуть NULL. Владение не передаётся. */
FVM_PortFile *dvm_runtime_file(int descriptor)
{
    fvm_port_registry_lock();
    FVM_PortFile *file = valid(descriptor) ? dvm_vfs_file(&slots[descriptor].resource) : NULL;
    fvm_port_registry_unlock();
    return file;
}


/* EN: Borrow descriptor's gate while caller's rooted FD/loader pins its lifetime.
 * Return NULL for stdio/invalid slots; never access an unowned descriptor.
 * RU: Заимствовать шлюз descriptor, пока корень FD/загрузчик держит его жизнь.
 * Для stdio/неверного слота вернуть NULL; чужой дескриптор не допускается. */
DVM_FileGate *dvm_runtime_file_gate(int descriptor)
{
    fvm_port_registry_lock();
    DVM_FileGate *gate = valid(descriptor) ? slots[descriptor].gate : NULL;
    fvm_port_registry_unlock();
    return gate;
}


/* EN: Borrow descriptor's directory under the mutex with FD/loader lifetime pin.
 * Return NULL for stdio, reserved or non-directory slots. No ownership transfer.
 * RU: Заимствовать каталог descriptor под мьютексом с удержанием FD/загрузчиком.
 * Для stdio, резерва или не-каталога вернуть NULL. Владение не передаётся. */
FVM_PortDirectory *dvm_runtime_directory(int descriptor)
{
    fvm_port_registry_lock();
    FVM_PortDirectory *directory = valid(descriptor) ? dvm_vfs_directory(&slots[descriptor].resource) : NULL;
    fvm_port_registry_unlock();
    return directory;
}


/* EN/RU: See file_slots.h; rooted caller excludes detachment/reuse until return.
 * См. file_slots.h; rooted caller исключает detachment/reuse до возврата. */
DVM_VfsHandle *dvm_runtime_resource(int descriptor)
{
    DVM_VfsHandle *handle;
    fvm_port_registry_lock();
    handle = valid(descriptor) && slots[descriptor].resource.ops ? &slots[descriptor].resource : NULL;
    fvm_port_registry_unlock();
    return handle;
}
