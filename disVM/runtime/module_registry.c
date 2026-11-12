/* Copyright (c) 2026 bhgv
 * EN: Protect shared Module list membership together with lookup/last release.
 * RU: Защитить общий список Module вместе с поиском и снятием последней ссылки.
 */
#include "lib9.h"
#include "interp.h"
#include "module_registry.h"
#include "module_counts.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <limits.h>
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


/* [GC:] EN: Serialize the zero-count claim with lookup. Entries are owned and
 * unique; owners supplies expected native counts, or NULL for one each.
 * argument belongs to the bounded native claim callback. No native or
 * managed destructor runs under this mutex. Failure leaves membership intact.
 * RU: Сериализовать захват нуля с lookup. Entries принадлежат вызывающему и
 * уникальны; owners задаёт числа ссылок либо NULL для одной. argument передаётся
 * ограниченному native callback. Под mutex нет
 * деструкторов. При отказе состав списка не меняется. */
int dvm_module_registry_claim_counts(
    Module **entries, const unsigned int *owners, unsigned count, int (*claim)(void *), void *argument)
{
    pthread_mutex_lock(&registry_mutex);
    for (unsigned index = 0; index < count; index++)
    {
        unsigned int expected = owners ? owners[index] : 1;
        if (!entries[index] || !expected || expected > INT_MAX ||
            dvm_refcount_load_i(&entries[index]->ref) != (int)expected)
        {
            pthread_mutex_unlock(&registry_mutex);
            return 0;
        }
    }
    int claimed = claim(argument);
    if (claimed)
    {
        for (unsigned index = 0; index < count; index++)
        {
            Module *module = entries[index];
            if (dvm_refcount_load_i(&module->ref) != 0)
            {
                abort();
            }
            Module **slot = &modules;
            while (*slot && *slot != module)
            {
                slot = &(*slot)->link;
            }
            if (*slot)
            {
                *slot = module->link;
            }
            module->link = NULL;
        }
    }
    pthread_mutex_unlock(&registry_mutex);
    return claimed;
}


/* [GC:] EN: Preserve the single-owner claim interface for callers with unique
 * native owners. entries/count/claim/argument follow the counted claim contract.
 * RU: Сохранить интерфейс захвата единственных владельцев. entries/count/claim/
 * argument следуют контракту захвата с заданным числом native-ссылок. */
int dvm_module_registry_claim(Module **entries, unsigned count, int (*claim)(void *), void *argument)
{
    return dvm_module_registry_claim_counts(entries, NULL, count, claim, argument);
}
