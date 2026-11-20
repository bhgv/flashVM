/* Copyright (c) 2026 bhgv
 * EN: Open exact numbered terminal files and retain their shared cursors per launch.
 * RU: Открытие номерных файлов терминала и удержание общей позиции для запуска.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "runqueue.h"
#include "launch_files.h"
#include <limits.h>
#include <stdlib.h>


/* EN/RU: Immutable mapping; each occupied entry owns one native slot reference.
 * Неизменяемая таблица; каждая занятая запись владеет одной ссылкой на слот. */
struct DVM_LaunchFiles
{
    unsigned references;            /* Atomic ownership count through process inheritance. */
    uint32_t occupied;              /* Mask of imported guest numbers. */
    int slots[DVM_LAUNCH_FD_COUNT]; /* Native slots; zero when absent. */
};


/* EN/RU: Stat one exact grant before consuming any one-use file in the batch.
 * Проверить метаданные точной выдачи до открытия одноразовых файлов. */
static int expected_file(const DVM_TerminalChannels *terminal, unsigned number)
{
    char absolute[DVM_LAUNCH_FD_PATH_SIZE];
    if (!dvm_launch_fd_path(absolute, number))
    {
        return 0;
    }
    const char *path = absolute + 1;
    FVM_FileInfo info;
    return strcmp(path, terminal->receive) && strcmp(path, terminal->send) &&
        dvm_provider_info(terminal->provider, path, &info) == FVM_OK && !info.directory;
}


/* EN/RU: Reserve lifetime storage, then open the exact terminal file without creation.
 * Зарезервировать память, затем открыть точный файл терминала без создания. */
static int open_slot(const DVM_TerminalChannels *terminal, unsigned number, unsigned access)
{
    char absolute[DVM_LAUNCH_FD_PATH_SIZE];
    if (!dvm_launch_fd_path(absolute, number))
    {
        return -1;
    }
    const char *mode = access == FVM_ACCESS_READ ? "r" : access == FVM_ACCESS_WRITE ? "w" : "rw";
    DVM_FileGate *gate = dvm_file_gate_create();
    DVM_CloseJob *job = gate ? dvm_close_prepare() : NULL;
    int slot = job ? dvm_file_slots_reserve_launch(gate, job) : -1;
    if (slot < 0)
    {
        dvm_file_gate_destroy(gate);
        dvm_close_discard(job);
        return -1;
    }
    DVM_VfsHandle file = {0};
    FVM_Result status = dvm_terminal_channel_open(terminal, absolute + 1, mode, &file);
    if (status != FVM_OK)
    {
        dvm_file_slots_drop_launch(slot);
        return -1;
    }
    dvm_file_slots_publish_resource(slot, &file);
    return slot;
}


/* EN/RU: See launch_files.h; validate all requested modes before taking any open.
 * См. launch_files.h; проверить все режимы до получения открытых файлов. */
int dvm_launch_files_open(const DVM_LaunchHandoff *handoff, DVM_LaunchFiles **out)
{
    *out = NULL;
    uint32_t requested = 0;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        unsigned mode = handoff->inherited_fds[i];
        if (mode & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE))
        {
            return 0;
        }
        if (mode)
        {
            requested |= UINT32_C(1) << i;
        }
    }
    if (!requested)
    {
        return 1;
    }
    if (!dvm_terminal_channels_valid(&handoff->flashvm))
    {
        return 0;
    }
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        if (handoff->inherited_fds[i] && !expected_file(&handoff->flashvm, i))
        {
            return 0;
        }
    }
    DVM_LaunchFiles *files = calloc(1, sizeof(*files));
    if (!files)
    {
        return 0;
    }
    files->references = 1;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        if (requested & (UINT32_C(1) << i))
        {
            int slot = open_slot(&handoff->flashvm, i, handoff->inherited_fds[i]);
            if (slot < 0)
            {
                dvm_launch_files_release(files);
                return 0;
            }
            files->slots[i] = slot;
        }
    }
    files->occupied = requested;
    *out = files;
    return 1;
}


/* EN/RU: See launch_files.h; an existing caller pin prevents resurrection.
 * См. launch_files.h; ссылка вызывающего исключает повторное оживление. */
int dvm_launch_files_retain(DVM_LaunchFiles *files)
{
    if (!files)
    {
        return 1;
    }
    unsigned count = __atomic_load_n(&files->references, __ATOMIC_RELAXED);
    while (count && count != UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&files->references, &count, count + 1, 0,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See launch_files.h; retirement also accepts a partially opened table.
 * См. launch_files.h; освобождение принимает и частично заполненную таблицу. */
void dvm_launch_files_release(DVM_LaunchFiles *files)
{
    if (files && __atomic_sub_fetch(&files->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
        {
            if (files->slots[i])
            {
                dvm_file_slots_drop_launch(files->slots[i]);
            }
        }
        free(files);
    }
}


/* EN/RU: Borrow the native table pinned by the current process, or NULL during boot.
 * Заимствовать таблицу текущего процесса, либо NULL при boot. */
static DVM_LaunchFiles *current(void)
{
    Prog *process = currun();
    return process ? ((DVM_RuntimeProcess *)process)->launch_files : NULL;
}


/* EN/RU: See launch_files.h; the current process pins the returned slot.
 * См. launch_files.h; текущий процесс удерживает возвращённый слот. */
int dvm_launch_file_current(int number)
{
    DVM_LaunchFiles *files = current();
    return files && number >= 0 && (unsigned)number < DVM_LAUNCH_FD_COUNT && files->slots[number] ?
        files->slots[number] : -1;
}


/* EN/RU: See launch_files.h; numbers are immutable for this process lifetime.
 * См. launch_files.h; набор номеров неизменен в течение жизни процесса. */
uint32_t dvm_launch_file_mask(void)
{
    DVM_LaunchFiles *files = current();
    return files ? files->occupied : 0;
}


/* EN/RU: See launch_files.h; the native suffix is set only during FD construction.
 * См. launch_files.h; нативный суффикс задаётся только при создании FD. */
int dvm_sys_fd_slot(Sys_FD *fd)
{
    int imported = ((DVM_SysFD *)fd)->launch_slot;
    if (imported)
    {
        return imported;
    }
    return fd->fd >= 0 && fd->fd < DVM_LAUNCH_SLOT_FIRST ? (int)fd->fd : -1;
}
