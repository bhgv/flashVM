/* Copyright (c) 2026 bhgv
 * EN: Mount host-backed directories in the ordinary FlashVM file namespace.
 * RU: Монтирование каталогов хоста в обычном пространстве файлов FlashVM.
 */
#ifndef FLASHVM_FILESYSTEM_H
#define FLASHVM_FILESYSTEM_H
#include "flashvm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FVM_FsInfo
{
    unsigned long length_high; /* High 32 bits of the byte length. */
    unsigned long length_low;  /* Low 32 bits of the byte length. */
    unsigned long mode;        /* Low nine permission bits. */
    int directory;             /* Nonzero for a directory. */
} FVM_FsInfo;

typedef FVM_Result (*FVM_FsVisit)(void *state, const char *name, const FVM_FsInfo *info); /* Borrowed entry. */

typedef struct FVM_FsFile
{
    FVM_Result (*read)(void *, long, void *, size_t, size_t *); /* Read bytes at a checked offset. */
    FVM_Result (*write)(void *, long, const void *, size_t);    /* Write all bytes or fail without replay. */
    FVM_Result (*size)(void *, size_t *);                      /* Query the size on the held file. */
    void (*close)(void *);                                    /* Consume handle, including in GC cleanup. */
} FVM_FsFile;

typedef struct FVM_Filesystem
{
    FVM_FsFile file; /* Operations copied into each open descriptor. */
    FVM_Result (*open)(void *, const char *, const char *, void **); /* Transfer a file in the requested mode. */
    FVM_Result (*stat)(void *, const char *, FVM_FsInfo *);          /* Read metadata at a relative path. */
    FVM_Result (*list)(void *, const char *, size_t, size_t, FVM_FsVisit, void *); /* Visit a page of child entries. */
} FVM_Filesystem;


/* EN: Mount backend at one absolute component such as /shared, or at / in a fresh namespace.
 * The root mount is exclusive and lasts until destruction. Copy callbacks and path.
 * Borrow state through unmount or context destruction. Roots and handles stay owner-local.
 * RU: Смонтировать backend по одному компоненту вроде /shared либо в / нового пространства.
 * Корневое монтирование исключительное и действует до уничтожения.
 * Скопировать колбэки и путь. Заимствовать state до снятия монтирования или уничтожения контекста. */
FVM_API FVM_Result fvm_filesystem_mount(FVM_Context *context, const char *path,
    const FVM_Filesystem *backend, void *state);


/* EN: Remove the path binding. Existing handles keep their original backend and identity.
 * RU: Снять привязку пути. Открытые файлы сохраняют исходный backend и идентичность. */
FVM_API FVM_Result fvm_filesystem_unmount(FVM_Context *context, const char *path);


/* [GC:] EN: Install the ordinary ActionScript fs namespace under a free global name.
 * RU: Установить обычное пространство fs в свободное глобальное имя ActionScript. */
FVM_API FVM_Result fvm_filesystem_install(FVM_Context *context, const char *global);

#ifdef __cplusplus
}
#endif
#endif
