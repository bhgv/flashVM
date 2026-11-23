/* Copyright (c) 2026 bhgv
 * EN: Compose the reverse Styx tree for a module's caller contexts.
 * RU: Сборка встречного дерева Styx с контекстами клиентов модуля.
 */
#ifndef DVM_PROGRAM_VIEW_H
#define DVM_PROGRAM_VIEW_H
#include "launch/fd_path.h"
#include "provider.h"
#include "namespace.h"
#include "launch/origin.h"
#include <stdint.h>


/* EN/RU: Grants are borrowed directory roots; a NULL grant omits that subtree.
 * Каталоги заимствуются; NULL исключает соответствующее поддерево. */
typedef struct DVM_ProgramView
{
    uint32_t pid; /* Nonzero PID in the publishing VM. */
    DVM_Provider *fds; /* Numbered file grants, preserving their open modes. */
    DVM_Provider *vfs; /* Granted namespace subtree, including ordinary env files. */
    DVM_Provider *events; /* Optional read-only event stream for the published grants. */
    DVM_Provider *vars; /* Granted shared variable proxies. */
    DVM_Provider *unibus; /* Optional private UniBus endpoint for this call. */
} DVM_ProgramView;


/* EN: Return an owned per-module tree containing distinct PIDs and retained grants.
 * The input array is borrowed through return. Invalid or duplicate entries fail
 * the whole build. No process lookup or namespace expansion is performed.
 * RU: Вернуть собственное дерево модуля с разными PID и удерживаемыми каталогами.
 * Массив заимствуется до возврата. Ошибки и повторы отменяют всю сборку.
 * Поиска процессов и расширения полномочий пространства имён нет. */
DVM_Provider *dvm_program_view(const DVM_ProgramView *programs, size_t count);


/* EN: Capture one PID directory in the borrowed module tree. The result owns
 * its exact identity and remains valid independently of the input reference.
 * RU: Захватить каталог PID из заимствованного дерева модуля. Результат
 * удерживает точный каталог независимо от ссылки на исходное дерево. */
DVM_Provider *dvm_program_capture(DVM_Provider *root, uint32_t pid);

typedef struct DVM_ProgramPublication DVM_ProgramPublication; /* Control borrowed from a live root. */


/* EN: Return an owned empty stable module root and a borrowed update control.
 * Keep a root reference for the lifetime of every update. Failure clears control.
 * RU: Вернуть свой пустой корень модуля и заимствованное управление.
 * На время обновления удерживать корень. Ошибка обнуляет управление. */
DVM_Provider *dvm_program_publication(DVM_ProgramPublication **control);


/* EN: Atomically replace the published entries. A zero count clears the root.
 * The array is borrowed for this call. Failure preserves the current generation.
 * Previously captured PID directories stay valid. Concurrent updates publish in
 * commit order. Provider callbacks may block while constructing a replacement.
 * RU: Атомарно заменить опубликованные записи. Нулевой count очищает корень.
 * Массив заимствуется. Ошибка сохраняет текущее поколение, прошлые захваты
 * остаются действительными. Параллельные обновления применяются в порядке
 * фиксации. Сборка нового дерева может ждать провайдеров. */
int dvm_program_publication_replace(DVM_ProgramPublication *control,
    const DVM_ProgramView *entries, size_t count);
struct DVM_LaunchFiles; /* Borrowed numbered table pinned by the active caller. */


/* EN: Capture an optional directory by name under a pinned program. On FVM_OK
 * writes an owned grant, on FVM_NOT_FOUND leaves out unchanged. Lookup errors propagate.
 * RU: Захватить необязательный каталог по имени в удерживаемом контексте.
 * FVM_OK выдаёт свою ссылку, FVM_NOT_FOUND не меняет out. Ошибки поиска возвращаются. */
FVM_Result dvm_program_child(DVM_Provider *program, const char *name, DVM_Provider **out);


/* EN: Publish this process's selected grants during native wait. Explicit mounts
 * override incoming call grants or, for jobs, inherited launch resources.
 * RU: Опубликовать выданные ресурсы процесса при ожидании. Явные маунты имеют
 * приоритет над входящим контекстом вызова или унаследованными ресурсами запуска. */
DVM_Provider *dvm_program_current(DVM_Namespace *group, struct DVM_LaunchFiles *descriptors, uint32_t pid);


/* EN: Same current-process grants, also retaining an optional .unibus file.
 * RU: Те же полномочия текущего процесса с удержанием необязательного .unibus. */
DVM_Provider *dvm_program_current_unibus(DVM_Namespace *group, struct DVM_LaunchFiles *descriptors,
    uint32_t pid, DVM_Provider *unibus);


/* EN: Return an owned private .prog root over one borrowed context directory.
 * The verified PID and .self name the same grant. PID zero supplies only .self
 * for a legacy context. Failure returns NULL without retaining the input.
 * RU: Вернуть свой закрытый корень .prog поверх заимствованного каталога.
 * Проверенный PID и .self именуют одну выдачу. При PID ноль для старого
 * контекста создаётся только .self. При отказе входная ссылка не удерживается. */
DVM_Provider *dvm_program_aliases(DVM_Provider *context, uint32_t pid);


/* EN: Stage a borrowed directory at an absolute virtual path in an exclusive,
 * unpublished namespace. Fail if a mount already occupies the path or its descendants.
 * RU: Поместить заимствованный каталог по виртуальному абсолютному пути в
 * неопубликованном namespace. Отказать при существующем маунте в этом поддереве. */
int dvm_program_mount_private(DVM_Namespace *group, DVM_Provider *provider, const char *target,
    const DVM_Origin *origin);

/* EN: Capture standard descriptors and the borrowed table of host FDs by guest number.
 * Entries below 3 leave the corresponding slot unspecified. Returns an owned provider.
 * RU: Захватить стандартные дескрипторы и таблицу хостовых FD по гостевому номеру.
 * Значения меньше 3 пропускают явное назначение. Возвращает свою ссылку. */
DVM_Provider *dvm_launch_host_fd_view(const int descriptors[DVM_LAUNCH_FD_COUNT], int *success);

#endif
