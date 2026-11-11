/* Copyright (c) 2026 bhgv
 * EN: Protect shared Module list membership together with lookup/last release.
 * RU: Защитить общий список Module вместе с поиском и снятием последней ссылки.
 */
#include "lib9.h"
#include "interp.h"
#include "module_registry.h"
#include "module_counts.h"
#include <pthread.h>
#include <string.h>

static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER; /* Module membership, not VM execution. */


/* EN: Publish borrowed entries atomically; callers retain their initial owners.
 * Registry metadata only: never allocate, perform I/O or run destructors locked.
 * RU: Атомарно опубликовать entries; вызывающий сохраняет исходное владение.
 * Только метаданные: без выделений, I/O или деструкторов под mutex. */
void dvm_module_registry_publish_set(Module **entries, unsigned count)
{
    pthread_mutex_lock(&registry_mutex);
    unsigned index;
    for (index = 0; index < count; index++)
    {
        entries[index]->link = modules;
        modules = entries[index];
    }
    pthread_mutex_unlock(&registry_mutex);
}


/* EN: Publish one fully initialized borrowed module without changing references.
 * RU: Опубликовать один готовый заимствованный module без изменения ссылок. */
void dvm_module_registry_publish(Module *module)
{
    dvm_module_registry_publish_set(&module, 1);
}


/* [GC:] EN: Acquire a reference while path's matching list entry is protected.
 * No pointer can escape between last release and removal. Return owned or NULL.
 * RU: Получить ссылку на запись path под защитой списка. Указатель не выходит
 * между последним release и удалением записи. Вернуть владение либо NULL. */
Module *dvm_module_registry_lookup(const char *path)
{
    pthread_mutex_lock(&registry_mutex);
    Module *module;
    for (module = modules; module; module = module->link)
    {
        if (!strcmp(path, module->path))
        {
            dvm_runtime_module_retain(&module->ref);
            break;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    return module;
}


/* [GC:] EN: Release module and unlink only LAST under the lookup mutex. After
 * unlock, the caller alone may destroy metadata, MP, types and imported modules.
 * RU: Снять ссылку module и удалить только LAST под mutex поиска. После снятия
 * mutex только вызывающий может уничтожить метаданные, MP, типы и импорты. */
int dvm_module_registry_release(Module *module)
{
    pthread_mutex_lock(&registry_mutex);
    int last = dvm_runtime_module_release(&module->ref);
    if (last)
    {
        Module **slot;
        for (slot = &modules; *slot && *slot != module; slot = &(*slot)->link)
        {
        }
        if (*slot)
        {
            *slot = module->link;
        }
        module->link = NULL;
    }
    pthread_mutex_unlock(&registry_mutex);
    return last;
}
