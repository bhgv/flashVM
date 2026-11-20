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
#include <string.h>


/* EN/RU: Immutable mapping; each occupied entry owns one native slot reference.
 * Неизменяемая таблица; каждая занятая запись владеет одной ссылкой на слот. */
struct DVM_LaunchFiles
{
    unsigned references;            /* Atomic ownership count through process inheritance. */
    uint32_t hidden;                /* Explicitly absent numbers reserved in the launch view. */
    uint32_t occupied;              /* Mask of imported guest numbers. */
    unsigned char modes[DVM_LAUNCH_FD_COUNT]; /* Original access of each imported file. */
    int slots[DVM_LAUNCH_FD_COUNT]; /* Native slots; zero when absent. */
};


/* EN/RU: Parse a canonical decimal basename in the supported descriptor range.
 * Разобрать каноническое десятичное имя в диапазоне поддерживаемых номеров. */
static int fd_number(const char *name)
{
    if (name[0] < '0' || name[0] > '9')
    {
        return -1;
    }
    unsigned number = (unsigned)(name[0] - '0');
    if (name[1])
    {
        if (!number || name[1] < '0' || name[1] > '9' || name[2])
        {
            return -1;
        }
        number = number * 10 + (unsigned)(name[1] - '0');
    }
    return number < DVM_LAUNCH_FD_COUNT ? (int)number : -1;
}


/* EN/RU: Prove absence by listing the root when a peer's lookup error is ambiguous.
 * Подтвердить отсутствие через чтение корня при неоднозначном ответе поиска. */
static FVM_Result confirm_absence(DVM_Provider *source)
{
    DVM_VfsHandle root = {0};
    FVM_Result status = dvm_provider_open(source, ".", "r", &root);
    if (status != FVM_OK)
    {
        return FVM_ERROR;
    }
    size_t count;
    do
    {
        FVM_FileInfo *entries = NULL;
        count = 0;
        status = dvm_vfs_list(&root, &entries, &count);
        for (size_t i = 0; status == FVM_OK && i < count; i++)
        {
            if (!strcmp(entries[i].name, ".fd"))
            {
                status = FVM_ERROR;
            }
        }
        fvm_port_directory_entries_free(entries);
    }
    while (status == FVM_OK && count);
    dvm_vfs_close(&root);
    return status == FVM_OK ? FVM_NOT_FOUND : FVM_ERROR;
}


/* EN/RU: Read a complete bounded directory into the private mode table.
 * Прочитать ограниченный каталог целиком в частную таблицу режимов. */
static FVM_Result discover(DVM_Provider *source, unsigned char *modes)
{
    if (!source)
    {
        return FVM_NOT_FOUND;
    }
    FVM_FileInfo info;
    FVM_Result status = dvm_provider_info(source, ".fd", &info);
    if (status == FVM_ERROR)
    {
        return confirm_absence(source);
    }
    if (status != FVM_OK)
    {
        return status;
    }
    DVM_VfsHandle directory = {0};
    if (!info.directory || dvm_provider_open(source, ".fd", "r", &directory) != FVM_OK)
    {
        return FVM_ERROR;
    }
    size_t count;
    do
    {
        FVM_FileInfo *entries = NULL;
        count = 0;
        status = dvm_vfs_list(&directory, &entries, &count);
        for (size_t i = 0; status == FVM_OK && i < count; i++)
        {
            const FVM_FileInfo *entry = &entries[i];
            int number = fd_number(entry->name);
            unsigned mode = ((entry->mode & 0444) ? FVM_ACCESS_READ : 0) |
                            ((entry->mode & 0222) ? FVM_ACCESS_WRITE : 0);
            if (number < 0 || modes[number] || entry->directory || !mode)
            {
                status = FVM_ERROR;
            }
            else
            {
                modes[number] = (unsigned char)mode;
            }
        }
        fvm_port_directory_entries_free(entries);
    }
    while (status == FVM_OK && count);
    dvm_vfs_close(&directory);
    return status;
}


/* EN/RU: Reserve lifetime storage and open one numbered file without truncation.
 * Зарезервировать время жизни и открыть один номерной файл без усечения. */
static int open_slot(DVM_Provider *source, unsigned number, unsigned access)
{
    char absolute[DVM_LAUNCH_FD_PATH_SIZE];
    if (!dvm_launch_fd_path(absolute, number))
    {
        return -1;
    }
    const char *mode = access == FVM_ACCESS_READ ? "r" : access == FVM_ACCESS_WRITE ? "wo" : "rw";
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
    FVM_FileInfo info;
    if (dvm_provider_open(source, absolute + 1, mode, &file) != FVM_OK ||
        dvm_vfs_info(&file, &info) != FVM_OK || info.directory)
    {
        dvm_vfs_close(&file);
        dvm_file_slots_drop_launch(slot);
        return -1;
    }
    dvm_file_slots_publish_resource(slot, &file);
    return slot;
}


/* EN/RU: Validate the selected batch and keep every open private until success.
 * Проверить выдачу и удерживать все открытия в частной таблице до успеха. */
static int open_batch(DVM_Provider *source, DVM_Provider *base, uint32_t from_base,
    uint32_t hidden, const DVM_TerminalChannels *terminal, const unsigned char *modes, DVM_LaunchFiles **out)
{
    uint32_t requested = 0;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        unsigned mode = modes[i];
        uint32_t bit = UINT32_C(1) << i;
        DVM_Provider *selected = from_base & bit ? base : source;
        if (!mode || (hidden & bit))
        {
            continue;
        }
        char absolute[DVM_LAUNCH_FD_PATH_SIZE];
        FVM_FileInfo info;
        if ((mode & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE)) ||
            !dvm_launch_fd_path(absolute, i) || !selected ||
            (terminal && (!strcmp(absolute + 1, terminal->receive) || !strcmp(absolute + 1, terminal->send))) ||
            dvm_provider_info(selected, absolute + 1, &info) != FVM_OK || info.directory)
        {
            return 0;
        }
        requested |= bit;
    }
    if (!requested && !hidden)
    {
        return 1;
    }
    DVM_LaunchFiles *files = calloc(1, sizeof(*files));
    if (!files)
    {
        return 0;
    }
    files->references = 1;
    files->hidden = hidden;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        uint32_t bit = UINT32_C(1) << i;
        if (requested & bit)
        {
            DVM_Provider *selected = from_base & bit ? base : source;
            int slot = open_slot(selected, i, modes[i]);
            if (slot < 0)
            {
                dvm_launch_files_release(files);
                return 0;
            }
            files->slots[i] = slot;
            files->modes[i] = modes[i];
        }
    }
    files->occupied = requested;
    *out = files;
    return 1;
}


/* EN/RU: Overlay per-number replacement and omission before opening any FD.
 * Применить замены и исключения по номерам до открытия дескрипторов. */
static int open_program(const DVM_LaunchHandoff *handoff, DVM_LaunchFiles **out)
{
    unsigned char modes[DVM_LAUNCH_FD_COUNT] = {0};
    unsigned char inherited[DVM_LAUNCH_FD_COUNT] = {0};
    FVM_Result selected = discover(handoff->program, modes);
    FVM_Result base = discover(handoff->program_parent, inherited);
    if ((selected != FVM_OK && selected != FVM_NOT_FOUND) ||
        (base != FVM_OK && base != FVM_NOT_FOUND))
    {
        return 0;
    }
    uint32_t from_base = 0;
    uint32_t hidden = handoff->program_hidden_fds;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        if (!modes[i] && inherited[i])
        {
            modes[i] = inherited[i];
            from_base |= UINT32_C(1) << i;
        }
        if (i < 3 && !modes[i])
        {
            hidden |= UINT32_C(1) << i;
        }
    }
    return open_batch(handoff->program, handoff->program_parent, from_base,
        hidden, NULL, modes, out);
}


/* EN/RU: Prefer the owner's .fd directory. Use the accepted terminal only on absence.
 * Сначала каталог .fd владельца. Принятый терминал используется только при отсутствии. */
int dvm_launch_files_open(const DVM_LaunchHandoff *handoff, DVM_LaunchFiles **out)
{
    *out = NULL;
    if (handoff->program)
    {
        return open_program(handoff, out);
    }
    const DVM_TerminalChannels *terminal = &handoff->flashvm;
    for (unsigned i = 0; i < DVM_LAUNCH_FD_COUNT; i++)
    {
        if (handoff->inherited_fds[i])
        {
            return dvm_terminal_channels_valid(terminal) &&
                open_batch(terminal->provider, NULL, 0, 0, terminal, handoff->inherited_fds, out);
        }
    }
    unsigned char modes[DVM_LAUNCH_FD_COUNT] = {0};
    DVM_Provider *owner = dvm_launch_handoff_owner_root(handoff);
    FVM_Result status = discover(owner, modes);
    int opened = status == FVM_OK && open_batch(owner, NULL, 0, 0, NULL, modes, out);
    dvm_provider_release(owner);
    if (status != FVM_NOT_FOUND)
    {
        return opened;
    }
    status = discover(terminal->provider, modes);
    return status == FVM_NOT_FOUND ||
        (status == FVM_OK && open_batch(terminal->provider, NULL, 0, 0, terminal, modes, out));
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
    return files ? files->occupied | files->hidden : 0;
}


/* EN/RU: Check whether the launch explicitly hides the requested number.
 * Проверить, скрывает ли запуск запрошенный номер. */
int dvm_launch_file_hidden(unsigned number)
{
    DVM_LaunchFiles *files = current();
    return dvm_launch_file_is_hidden(files, number);
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


/* EN/RU: See launch_files.h. Copy the immutable mode and slot from a pinned table.
 * См. launch_files.h. Прочитать режим и слот из удерживаемой таблицы. */
int dvm_launch_file_lookup(DVM_LaunchFiles *files, unsigned number, unsigned *access)
{
    *access = files && number < DVM_LAUNCH_FD_COUNT ? files->modes[number] : 0;
    return *access ? files->slots[number] : -1;
}


/* EN/RU: See launch_files.h. Read only the immutable hidden mask.
 * См. launch_files.h. Читать только неизменяемую маску скрытых номеров. */
int dvm_launch_file_is_hidden(DVM_LaunchFiles *files, unsigned number)
{
    return files && number < DVM_LAUNCH_FD_COUNT && (files->hidden & (UINT32_C(1) << number));
}
