/* Copyright (c) 2026 bhgv
 * EN: Hold one real module open; optionally omit only file cache provenance.
 * RU: Задержать настоящий module open; опционально убрать только file cache provenance.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "filesystem_port.h"
#include "module_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int held_once;                      /* Atomic one-shot native fixture admission; no managed state. */
static FVM_FileNumber64 original_identity; /* First provider identity, seeded before victim spawn. */
static int identity_seeded;                /* Written before pthread publication; later reads are immutable. */
FVM_Result __real_fvm_port_file_open(const char *, const char *, FVM_PortFile **);
FVM_Result __real_fvm_port_file_info(FVM_PortFile *, FVM_FileInfo *);
Module *__real_dvm_module_registry_lookup(const char *);


/* EN/RU: Match selected physical source path without accessing VM data.
 * Сопоставить выбранный physical source path без чтения VM data. */
static int selected(const char *path)
{
    const char *suffix = "/source-a/provider.dis";
    size_t length = path ? strlen(path) : 0, tail = strlen(suffix);
    return length >= tail && !strcmp(path + length - tail, suffix);
}


/* EN/RU: Observe real cold/warm ownership before I/O; return unchanged Module ref.
 * Проверить настоящий cold/warm owner до I/O; вернуть неизменённую Module ref. */
Module *__wrap_dvm_module_registry_lookup(const char *path)
{
    Module *module = __real_dvm_module_registry_lookup(path);
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    if (process && process->pid > 1 && selected(path))
    {
        const char *warm = getenv("DVM_NAMESPACE_WARM");
        if (!warm || (module != NULL) != atoi(warm))
        {
            fputs("namespace fixture missed requested real cache ownership\n", stderr);
            exit(22);
        }
        fprintf(stderr, "NAMESPACELOADCACHE warm=%d\n", module != NULL);
    }
    return module;
}


/* EN: Preserve actual successful open before holding its source on an inherited
 * pipe. Input path/mode/output stay production-owned; another guest must rebind,
 * load and report progress before the controller releases this native owner.
 * RU: Сохранить настоящий successful open до задержки source на inherited pipe.
 * path/mode/output принадлежат production; сосед обязан rebind/load и сообщить
 * progress до освобождения controller этого native owner. */
FVM_Result __wrap_fvm_port_file_open(const char *path, const char *mode, FVM_PortFile **output)
{
    FVM_Result result = __real_fvm_port_file_open(path, mode, output);
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    int expected = 0;
    if (result == FVM_OK && process && process->pid > 1 && selected(path) &&
        __atomic_compare_exchange_n(&held_once, &expected, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
    {
        const char *descriptor = getenv("DVM_NAMESPACE_GATE");
        char byte;
        if (!descriptor)
        {
            exit(22);
        }
        fprintf(stderr, "NAMESPACELOADOPEN pid=%d\n", process->pid);
        if (read(atoi(descriptor), &byte, 1) != 1)
        {
            exit(22);
        }
    }
    return result;
}


/* EN: CONTROL1 recreates old zero identity/device/mtime metadata while preserving
 * real length/handles and application completion. Never linked into production.
 * RU: CONTROL1 повторяет старую zero identity/device/mtime, сохраняя настоящие
 * length/handles и завершение приложения. Отсутствует в production. */
FVM_Result __wrap_fvm_port_file_info(FVM_PortFile *file, FVM_FileInfo *output)
{
    FVM_Result result = __real_fvm_port_file_info(file, output);
    if ((CONTROL == 1 || CONTROL == 2) && result == FVM_OK)
    {
        int original = 0;
        if (CONTROL == 2)
        {
            if (!identity_seeded && !strcmp(output->name, "provider.dis"))
            {
                original_identity = output->identity;
                identity_seeded = 1;
            }
            original = identity_seeded && output->identity.high == original_identity.high &&
                       output->identity.low == original_identity.low;
        }
        memset(&output->identity, 0, sizeof(output->identity));
        memset(&output->device, 0, sizeof(output->device));
        memset(&output->modified, 0, sizeof(output->modified));
        if (CONTROL == 2 && original)
        {
            output->device.high = 0x10000UL;
        }
    }
    return result;
}
