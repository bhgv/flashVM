/* Copyright (c) 2026 bhgv
 * EN: Synchronized native descriptor reservation, publication and detachment.
 * RU: Синхронные резервация, публикация и отделение нативных дескрипторов.
 */
#include "file_slots.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DVM_FileSlot slots[32]; /* Registry mutex protects every access; 0..2 are stdio. */


/* EN: Return whether descriptor is in 3..31 without accessing registry storage.
 * RU: Вернуть принадлежность descriptor диапазону 3..31 без доступа к реестру. */
static int valid(int descriptor)
{
    return descriptor >= 3 && descriptor < 32;
}


/* EN: Reserve caller-owned gate/job atomically; see header for ownership/failure.
 * RU: Атомарно занять слот для gate/job; владение и отказ описаны в заголовке. */
int dvm_file_slots_reserve(DVM_FileGate *gate, DVM_CloseJob *job)
{
    if (!gate || !job)
    {
        fprintf(stderr, "disvm: invalid native slot reservation\n");
        abort();
    }
    int index;
    fvm_port_registry_lock();
    for (index = 3; index < 32; index++)
    {
        if (!slots[index].gate)
        {
            slots[index].gate = gate;
            slots[index].job = job;
            break;
        }
    }
    fvm_port_registry_unlock();
    return index == 32 ? -1 : index;
}


/* EN: Publish exactly one handle under the reservation's registry mutex.
 * descriptor belongs to this caller; file/directory transfer on success.
 * RU: Опубликовать ровно один handle под мьютексом резервации.
 * descriptor принадлежит вызывающему; владение file/directory передаётся. */
void dvm_file_slots_publish(int descriptor, FVM_PortFile *file, FVM_PortDirectory *directory)
{
    fvm_port_registry_lock();
    if (!valid(descriptor) || !slots[descriptor].gate || slots[descriptor].file ||
        slots[descriptor].directory || (file && directory) || (!file && !directory))
    {
        fvm_port_registry_unlock();
        fprintf(stderr, "disvm: invalid native slot publication\n");
        abort();
    }
    slots[descriptor].file = file;
    slots[descriptor].directory = directory;
    fvm_port_registry_unlock();
}


/* EN: Copy the exclusive descriptor into output and clear its entire registry slot.
 * published requires a ready handle. No lifecycle work occurs while locked.
 * RU: Скопировать личный descriptor в output и очистить весь слот реестра.
 * published требует готовый handle. Работа с жизнью ресурсов идёт вне мьютекса. */
int dvm_file_slots_take(int descriptor, int published, DVM_FileSlot *output)
{
    fvm_port_registry_lock();
    if (!valid(descriptor) || !slots[descriptor].gate ||
        (published && !slots[descriptor].file && !slots[descriptor].directory))
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
    FVM_PortFile *file = valid(descriptor) ? slots[descriptor].file : NULL;
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
    FVM_PortDirectory *directory = valid(descriptor) ? slots[descriptor].directory : NULL;
    fvm_port_registry_unlock();
    return directory;
}
