/* Copyright (c) 2026 bhgv
 * EN: Compose the reverse Styx tree for a module's caller contexts.
 * RU: Сборка встречного дерева Styx с контекстами клиентов модуля.
 */
#ifndef DVM_PROGRAM_VIEW_H
#define DVM_PROGRAM_VIEW_H
#include "provider.h"
#include <stdint.h>


/* EN/RU: Grants are borrowed directory roots; a NULL grant omits that subtree.
 * Каталоги заимствуются; NULL исключает соответствующее поддерево. */
typedef struct DVM_ProgramView
{
    uint32_t pid; /* Nonzero PID in the publishing VM. */
    DVM_Provider *fds; /* Numbered file grants, preserving their open modes. */
    DVM_Provider *vfs; /* Granted namespace subtree, including ordinary env files. */
    DVM_Provider *vars; /* Granted shared variable proxies. */
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
#endif
