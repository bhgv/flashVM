/* Copyright (c) 2026 bhgv
 * EN: Exercise real Module lookup versus final unlink/free with concurrent owners.
 * RU: Проверять настоящий поиск Module против последнего unlink/free при конкуренции.
 */
#include "lib9.h"
#include "interp.h"
#include "module_registry.h"
#include "disvm/refcount.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Module *modules;         /* This isolated executable owns its entire native registry. */
static unsigned created; /* Atomic constructed-owner count, observed after all joins. */
static unsigned freed;   /* Atomic count of unique last-owner destructions. */
static unsigned reads;   /* Atomic successful protected lookups. */
static int stopping;     /* Atomic publisher completion flag. */


/* EN: Require condition or terminate with borrowed diagnostic message.
 * RU: Проверить condition либо завершить с заимствованным диагностическим текстом. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL module registry: %s\n", message);
        exit(12);
    }
}


/* EN: Drop the borrowed owned reference; LAST alone frees native metadata.
 * No registry mutex remains held when this caller frees path/module storage.
 * RU: Снять владеющую ссылку; только LAST освобождает native-метаданные.
 * При освобождении path/module мьютекс реестра уже не удерживается. */
static void drop(Module *module)
{
    if (dvm_module_registry_release(module))
    {
        free(module->path);
        free(module);
        __atomic_fetch_add(&freed, 1, __ATOMIC_RELAXED);
    }
}


/* EN: Allocate an unpublished native Module with one owner and immutable payload.
 * path is borrowed and copied; return owned or fail on native allocation error.
 * RU: Создать личный Module с одной ссылкой и неизменным payload. path копируется;
 * вернуть владение либо завершить при ошибке native-выделения. */
static Module *make(const char *path)
{
    Module *module = calloc(1, sizeof(*module));
    require(module != NULL, "native Module allocation");
    size_t bytes = strlen(path) + 1;
    module->path = malloc(bytes);
    require(module->path != NULL, "native Module path allocation");
    memcpy(module->path, path, bytes);
    module->ref = 1;
    module->nprog = 42;
    __atomic_fetch_add(&created, 1, __ATOMIC_RELAXED);
    return module;
}


/* EN: Repeatedly acquire current native entry until stop; unused must be NULL.
 * Each successful lookup owns its metadata through the check and matching drop.
 * RU: Получать текущую запись до stop; unused равен NULL. Каждый успешный поиск
 * владеет метаданными при проверке и до соответствующего drop. */
static void *reader(void *unused)
{
    (void)unused;
    while (!__atomic_load_n(&stopping, __ATOMIC_ACQUIRE))
    {
        Module *module = dvm_module_registry_lookup("shared-native-module");
        if (module)
        {
            require(module->nprog == 42 && !strcmp(module->path, "shared-native-module"),
                    "lookup retains live immutable metadata");
            __atomic_fetch_add(&reads, 1, __ATOMIC_RELAXED);
            drop(module);
        }
        else
        {
            sched_yield();
        }
    }
    return NULL;
}


/* EN: Publish/drop 2500 independent owners while other publishers/readers run.
 * unused is NULL; every local module keeps one strong reference until drop.
 * RU: Опубликовать/снять 2500 владельцев при работе других писателей/читателей.
 * unused равен NULL; каждый личный module держит сильную ссылку до drop. */
static void *publisher(void *unused)
{
    (void)unused;
    unsigned index;
    for (index = 0; index < 2500; index++)
    {
        Module *module = make("shared-native-module");
        dvm_module_registry_publish(module);
        sched_yield();
        drop(module);
    }
    return NULL;
}


/* [GC:] EN: Native-only callback probe; managed graphs are tested in real apps.
 * RU: Native-проверка callback; managed-графы проверяются настоящими приложениями. */
typedef struct ClaimProbe
{
    Module *entries[2]; /* Borrowed stack-backed metadata through this probe. */
    unsigned calls;     /* Callback invocation count under registry mutex. */
    int commit;         /* Refuse after restoring the first count unless nonzero. */
} ClaimProbe;


/* [GC:] EN: Reserve first native count and either restore/refuse or reserve the
 * second. argument is private probe storage. No allocation or registry reentry.
 * RU: Захватить первый native-счётчик, затем откатить/отказать либо захватить
 * второй. argument — личное состояние. Без выделений и повторного входа в реестр. */
static int claim_pair(void *argument)
{
    ClaimProbe *probe = argument;
    probe->calls++;
    require(dvm_refcount_compare_i(&probe->entries[0]->ref, 1, 0), "first native claim");
    if (!probe->commit)
    {
        require(dvm_refcount_compare_i(&probe->entries[0]->ref, 0, 1), "native rollback before refusal");
        return 0;
    }
    require(dvm_refcount_compare_i(&probe->entries[1]->ref, 1, 0), "second native claim");
    return 1;
}


/* [GC:] EN: Check outside-owner refusal, callback rollback and batch unlink
 * using actual native counts/registry. Stack entries live until both are unlinked.
 * RU: Проверить отказ внешнему владельцу, откат callback и удаление пакета
 * настоящими счётчиками/реестром. Стековые записи живут до удаления обеих. */
static void check_claims(void)
{
    Module left = {0};
    Module right = {0};
    left.ref = 1;
    right.ref = 1;
    left.path = "claim-left";
    right.path = "claim-right";
    ClaimProbe probe = {{&left, &right}, 0, 0};
    dvm_module_registry_publish_set(probe.entries, 2);
    Module *outside = dvm_module_registry_lookup(left.path);
    require(outside == &left && !dvm_module_registry_claim(probe.entries, 2, claim_pair, &probe) && !probe.calls,
            "outside owner refuses before invoking count claim");
    require(!dvm_module_registry_release(outside), "drop only outside native owner");
    require(!dvm_module_registry_claim(probe.entries, 2, claim_pair, &probe) && probe.calls == 1 &&
                dvm_refcount_load_i(&left.ref) == 1 && dvm_refcount_load_i(&right.ref) == 1,
            "refused callback preserves native owners and membership");
    outside = dvm_module_registry_lookup(right.path);
    require(outside == &right && !dvm_module_registry_release(outside), "refused pair stays discoverable");
    probe.commit = 1;
    require(dvm_module_registry_claim(probe.entries, 2, claim_pair, &probe) && probe.calls == 2,
            "successful pair claim");
    require(!dvm_module_registry_lookup(left.path) && !dvm_module_registry_lookup(right.path) && !modules &&
                !left.link && !right.link && !dvm_refcount_load_i(&left.ref) && !dvm_refcount_load_i(&right.ref),
            "claimed native owners unlink before lookup resumes");
}


/* EN: Check batch publication, then race eight readers/four publishers with 10000 owners.
 * No VM/GC is mocked: this isolated test exercises metadata transactions only.
 * RU: Проверить пакетную публикацию и гонять восемь читателей/четыре писателя с 10000 владельцами.
 * VM/GC не подменяются: изолированный тест проверяет только native-метаданные. */
int main(void)
{
    check_claims();
    Module *batch[2];
    batch[0] = make("batch-left");
    batch[1] = make("batch-right");
    dvm_module_registry_publish_set(batch, 2);
    unsigned index;
    for (index = 0; index < 2; index++)
    {
        Module *found = dvm_module_registry_lookup(batch[index]->path);
        require(found == batch[index], "complete batch preserves entry identity");
        drop(found);
        drop(batch[index]);
    }
    pthread_t threads[8];
    for (index = 0; index < 8; index++)
    {
        require(!pthread_create(&threads[index], NULL, reader, NULL), "reader creation");
    }
    pthread_t writers[4];
    for (index = 0; index < 4; index++)
    {
        require(!pthread_create(&writers[index], NULL, publisher, NULL), "publisher creation");
    }
    for (index = 0; index < 4; index++)
    {
        require(!pthread_join(writers[index], NULL), "publisher join");
    }
    __atomic_store_n(&stopping, 1, __ATOMIC_RELEASE);
    for (index = 0; index < 8; index++)
    {
        require(!pthread_join(threads[index], NULL), "reader join");
    }
    require(modules == NULL && freed == created && reads > 0,
            "every published Module is unlinked/freed once after all readers finish");
    printf("PASS module registry owners=%u freed=%u readers=8 publishers=4 lookups=%u\n", created, freed, reads);
    return 0;
}
