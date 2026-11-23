/* Copyright (c) 2026 bhgv
 * EN: Register process context grants without allocating host or VM state.
 * RU: Регистрация ресурсов контекста процесса без аллокатора и VM.
 */
#ifndef STYXRPC_PROGRAM_H
#define STYXRPC_PROGRAM_H
#include "styxrpc/serve.h"
#include "styxrpc/variable_file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_FD_COUNT 32U /* Existing DisVM numbered-descriptor range. */


/* EN: File operations on a caller-supplied resource, with an optional per-open handle.
 * RU: Операции ресурса приложения с необязательным отдельным состоянием открытия. */
typedef struct StyxRpcResource
{
    void *context; /* Borrowed application state, used directly when open is absent. */
    int (*open)(void *context, unsigned access, void **cookie); /* Optional distinct handle; 1 on acceptance. */
    int (*read)(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size);
    int (*write)(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size);
    void (*close)(void *cookie); /* Called exactly once per successful open when present. */
} StyxRpcResource;


/* EN: One allocated registration slot. Applications supply storage and use the registration API.
 * RU: Слот регистрации. Приложение выделяет память и заполняет её через API. */
typedef struct StyxRpcRegistration
{
    StyxRpcEntry entry; /* Stable name and Qid. */
    uint64_t length;    /* Declared size in bytes. */
    uint64_t parent;    /* Containing context directory. */
    uint32_t pid;       /* Nonzero only on process directories and identity files. */
    char name[STYXRPC_TREE_NAME + 1]; /* Copied component. */
    StyxRpcResource resource;       /* Borrowed resource operations. */
    StyxIntVariable variable;       /* Signed scalar operations for a .var entry. */
    struct StyxIntArray *collection; /* Borrowed indexed collection, when present. */
    const void *collection_owner;   /* Session used to validate its selections. */
    struct StyxRpcRegistration *table_directory; /* Hybrid table command's parent slot. */
    const struct StyxRpcBigTable *table_spec;     /* Borrowed on the hybrid table directory. */
    void *table_selection;                       /* Active selector owned by the dispatcher. */
    struct StyxRpcProgramOpen *opens; /* Intrusive list of opened callback handles. */
    int revoked;                    /* Hidden slot reserved against stale Qids. */
    unsigned collection_op;         /* File operation within the collection. */
} StyxRpcRegistration;


/* EN: Application-owned fixed storage for one reverse session. Do not move it after initialization.
 * Register before serving or from the owning dispatcher, never concurrently.
 * RU: Фиксированная память приложения для встречной сессии. После инициализации не перемещать.
 * Регистрация до запуска или в потоке диспетчера, без конкурентных операций. */
typedef struct StyxRpcPrograms
{
    StyxRpcRegistration *nodes; /* Borrowed contiguous registration slots. */
    size_t capacity;            /* Slot count. */
    size_t used;                /* Occupied slots, including revoked entries. */
    uint32_t revision;          /* Revision of membership for this context tree. */
} StyxRpcPrograms;


/* EN: Initialize the root and .prog in fresh storage of capacity slots.
 * RU: Инициализировать корень и .prog в свежем массиве из capacity слотов. */
int styxrpc_programs_init(StyxRpcPrograms *programs, StyxRpcRegistration *storage, size_t capacity);


/* EN: Add a distinct nonzero PID plus .pid, .fd, .var and .vfs.
 * RU: Добавить уникальный ненулевой PID и .pid, .fd, .var, .vfs. */
int styxrpc_program_add(StyxRpcPrograms *programs, uint32_t pid);


/* EN: Register a numbered FD and its exact allowed access bits, plus byte metadata.
 * RU: Зарегистрировать FD с точными битами доступа и длиной в байтах. */
int styxrpc_program_fd(StyxRpcPrograms *programs, uint32_t pid, unsigned fd, unsigned access,
    uint64_t length, const StyxRpcResource *resource);


/* EN: Register a named signed scalar with separate read and write callbacks.
 * RU: Зарегистрировать именованную знаковую переменную с отдельными колбэками чтения и записи. */
int styxrpc_program_int(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxIntVariable *variable);


/* EN: Register a variable with caller-defined encoding and access callbacks.
 * RU: Зарегистрировать переменную с кодировкой и колбэками приложения. */
int styxrpc_program_variable(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);


/* EN: Add one file in the caller's .vfs root.
 * RU: Добавить файл в корень .vfs вызывающего. */
int styxrpc_program_file(StyxRpcPrograms *programs, uint32_t pid, const char *name, unsigned access,
    uint64_t length, const StyxRpcResource *resource);


/* EN: Publish the process's duplex .unibus callback resource once.
 * RU: Однократно опубликовать двусторонний ресурс .unibus процесса. */
int styxrpc_program_unibus(StyxRpcPrograms *programs, uint32_t pid, const StyxRpcResource *resource);


/* EN: Hide the registered .unibus and synchronously close its active callback handles.
 * Run on the dispatcher outside the target's callbacks. Return 1 on first revocation.
 * RU: Скрыть зарегистрированный .unibus и синхронно закрыть его активные колбэки.
 * Вызывать в диспетчере вне колбэков целевого ресурса. Первый отзыв возвращает 1. */
int styxrpc_program_unibus_revoke(StyxRpcPrograms *programs, uint32_t pid);


extern const StyxRpcTree styxrpc_program_tree; /* Pass StyxRpcPrograms as serve context. */

#ifdef __cplusplus
}
#endif
#endif
