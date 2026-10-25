/* Real native namespace tests: immutable readers, quotas, races and provider reentry. */
#include "disvm/module_store.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Per-thread operation owns only its result; store lifetime extends through join. */
typedef struct Race
{
    DVM_ModuleStore *store; /* Shared namespace using the real platform mutex. */
    unsigned long expected; /* Publication generation both writers compare. */
    DVM_StoreResult result; /* Result published by native thread join. */
    unsigned long next;     /* Output generation, unchanged on a failed CAS. */
    unsigned char byte;     /* Distinct complete candidate value for this writer. */
    FVM_PortSync *gate;     /* Real platform synchronization used to release both writers together. */
    int *ready;             /* Ready-thread count protected by gate. */
    int *go;                /* Release predicate protected by gate. */
} Race;


/* Report a failed expectation without disabling evaluation in release builds.
 * condition is already evaluated; message is a literal diagnostic. Returns condition. */
static int check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "module store: %s\n", message);
    }
    return condition;
}


/* Publish caller memory, then verify that an acquired lease owns a separate copy. */
static int versions(void)
{
    DVM_ModuleStore *store;
    DVM_ModuleLease *old = NULL;
    DVM_ModuleLease *current = NULL;
    DVM_ModuleView before;
    DVM_ModuleView after;
    DVM_ModuleStoreStats stats;
    DVM_ModuleEntry entries[2];
    DVM_ModuleImage image;
    unsigned char first[] = {1, 0, 2};
    unsigned char second[] = {4, 0, 5};
    unsigned long a = 0;
    unsigned long b = 0;
    size_t needed = 0;
    store = dvm_module_store_create(2, 6);
    image.bytes = first;
    image.size = sizeof(first);
    image.name = "/ide/Я.dis";
    if (!check(store != NULL, "create") ||
        !check(dvm_module_store_publish(store, &image, 0, &a) == DVM_STORE_OK, "first publication") ||
        !check(dvm_module_store_acquire(store, image.name, &old) == DVM_STORE_OK, "first lease") ||
        !check(dvm_module_lease_view(old, &before) == DVM_STORE_OK, "first view"))
    {
        return 0;
    }
    first[0] = 99;
    image.bytes = second;
    if (!check(before.bytes[0] == 1, "publication must copy source bytes") ||
        !check(dvm_module_store_publish(store, &image, a, &b) == DVM_STORE_OK && b > a, "new generation") ||
        !check(dvm_module_store_acquire(store, image.name, &current) == DVM_STORE_OK, "second lease") ||
        !check(dvm_module_lease_view(current, &after) == DVM_STORE_OK, "second view") ||
        !check(before.bytes[0] == 1 && after.bytes[0] == 4, "retired readers retain original bytes") ||
        !check(dvm_module_store_publish(store, &image, a, &a) == DVM_STORE_CONFLICT,
               "stale generation must not overwrite") ||
        !check(dvm_module_store_list(store, entries, 0, &needed) == DVM_STORE_LIMIT && needed == 1,
               "bounded listing reports needed capacity") ||
        !check(dvm_module_store_list(store, entries, 2, &needed) == DVM_STORE_OK &&
                   strcmp(entries[0].name, image.name) == 0 && entries[0].generation == b,
               "copied listing"))
    {
        return 0;
    }
    dvm_module_store_stats(store, &stats);
    if (!check(stats.versions == 2 && stats.bytes == 6 && stats.leases == 2, "retired bytes count toward quota") ||
        !check(dvm_module_store_publish(store, &image, b, &a) == DVM_STORE_LIMIT, "pinned-byte quota") ||
        !check(dvm_module_store_destroy(&store) == DVM_STORE_BUSY && store != NULL, "active leases block destruction"))
    {
        return 0;
    }
    dvm_module_lease_release(&old);
    dvm_module_lease_release(&old);
    dvm_module_lease_release(&current);
    if (!check(dvm_module_store_publish(store, &image, b, &a) == DVM_STORE_OK,
               "unleased replacement credits old bytes") ||
        !check(dvm_module_store_remove(store, image.name, b) == DVM_STORE_CONFLICT, "stale remove") ||
        !check(dvm_module_store_acquire(store, image.name, &current) == DVM_STORE_OK, "pin before remove") ||
        !check(dvm_module_lease_view(current, &after) == DVM_STORE_OK, "view before remove") ||
        !check(dvm_module_store_remove(store, image.name, a) == DVM_STORE_OK, "remove current name"))
    {
        return 0;
    }
    dvm_module_store_stats(store, &stats);
    if (!check(stats.names == 0 && stats.versions == 1 && stats.bytes == 3, "unpublished lease remains resident") ||
        !check(after.bytes[0] == 4, "view remains valid under its current lease"))
    {
        return 0;
    }
    dvm_module_store_close(store);
    if (!check(dvm_module_store_acquire(store, image.name, &old) == DVM_STORE_CLOSED, "closed acquisitions fail") ||
        !check(dvm_module_store_publish(store, &image, 0, &b) == DVM_STORE_CLOSED, "closed writes fail"))
    {
        return 0;
    }
    dvm_module_lease_release(&current);
    return check(dvm_module_store_destroy(&store) == DVM_STORE_OK && store == NULL, "destroy clears owner");
}


/* Reenter the same namespace while providing an image, proving the provider owns no store lock.
 * user borrows the store; name is requested identity; out receives owned bytes. */
static DVM_ImageResult provider(void *user, const char *name, DVM_ModuleImage *out)
{
    DVM_ModuleStoreStats stats;
    if (dvm_module_store_stats((DVM_ModuleStore *)user, &stats) != DVM_STORE_OK)
    {
        return DVM_IMAGE_IO;
    }
    return dvm_image_copy(name, "new", 3, 3, out);
}


/* A failed source may leave an allocation; the store must dispose it and preserve publication. */
static DVM_ImageResult failed_provider(void *user, const char *name, DVM_ModuleImage *out)
{
    DVM_ImageResult copied;
    (void)user;
    copied = dvm_image_copy(name, "partial", 7, 7, out);
    return copied == DVM_IMAGE_OK ? DVM_IMAGE_TRUNCATED : copied;
}


/* Exercise provider diagnostics, namespace validation and configured name capacity. */
static int sources(void)
{
    DVM_ModuleStore *store;
    DVM_ModuleImage image;
    DVM_ImageResult source = DVM_IMAGE_OK;
    unsigned long generation = 17;
    unsigned long output = 23;
    size_t index;
    const char *invalid[] = {"relative.dis", "/", "/a//b", "/a/../b", "/./a", "/a/", "/a\\b", "/a:b", "/a\nb"};
    store = dvm_module_store_create(1, 8);
    image.bytes = (unsigned char *)"old";
    image.size = 3;
    for (index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++)
    {
        image.name = (char *)invalid[index];
        if (!check(dvm_module_store_publish(store, &image, 0, &output) == DVM_STORE_INVALID && output == 23,
                   "invalid namespace must preserve output"))
        {
            return 0;
        }
    }
    image.name = "/main.dis";
    if (!check(dvm_module_store_publish(store, &image, 0, &generation) == DVM_STORE_OK, "source baseline") ||
        !check(dvm_module_store_load(store, image.name, generation, failed_provider, store, &source, &output) ==
                       DVM_STORE_SOURCE &&
                   source == DVM_IMAGE_TRUNCATED && output == 23,
               "source errors remain distinct") ||
        !check(dvm_module_store_load(store, image.name, generation, provider, store, &source, &output) ==
                       DVM_STORE_OK &&
                   source == DVM_IMAGE_OK && output > generation,
               "provider may reenter") ||
        !check(dvm_module_store_load(store, image.name, generation, provider, store, &source, &output) ==
                   DVM_STORE_CONFLICT,
               "provider completion still checks CAS"))
    {
        return 0;
    }
    image.name = "/other.dis";
    if (!check(dvm_module_store_publish(store, &image, 0, &generation) == DVM_STORE_LIMIT, "name quota"))
    {
        return 0;
    }
    return check(dvm_module_store_destroy(&store) == DVM_STORE_OK, "source store teardown");
}


/* Publish one complete scalar byte from a real OS thread; user borrows Race through join. */
static void writer(void *user)
{
    Race *race = (Race *)user;
    DVM_ModuleImage image;
    fvm_port_sync_lock(race->gate);
    (*race->ready)++;
    fvm_port_sync_signal_all(race->gate);
    while (!*race->go)
    {
        fvm_port_sync_wait(race->gate);
    }
    fvm_port_sync_unlock(race->gate);
    image.name = "/race.dis";
    image.bytes = &race->byte;
    image.size = 1;
    race->result = dvm_module_store_publish(race->store, &image, race->expected, &race->next);
}


/* Two writers compare the same generation; exactly one may commit on every iteration. */
static int races(void)
{
    DVM_ModuleStore *store;
    DVM_ModuleImage image;
    DVM_ModuleLease *lease = NULL;
    DVM_ModuleView view;
    Race workers[2];
    FVM_PortThread *threads[2];
    unsigned char byte = 0;
    unsigned long generation = 0;
    int iteration;
    int index;
    int ready = 0;
    int go = 0;
    FVM_PortSync *gate;
    store = dvm_module_store_create(1, 2);
    gate = fvm_port_sync_create();
    if (!check(gate != NULL, "native race barrier allocation"))
    {
        return 0;
    }
    image.name = "/race.dis";
    image.bytes = &byte;
    image.size = 1;
    if (!check(dvm_module_store_publish(store, &image, 0, &generation) == DVM_STORE_OK, "race baseline"))
    {
        return 0;
    }
    for (iteration = 0; iteration < 200; iteration++)
    {
        ready = 0;
        go = 0;
        for (index = 0; index < 2; index++)
        {
            workers[index].gate = gate;
            workers[index].ready = &ready;
            workers[index].go = &go;
            workers[index].store = store;
            workers[index].expected = generation;
            workers[index].byte = (unsigned char)(index + 1);
            workers[index].next = 0;
            threads[index] = fvm_port_thread_start(writer, &workers[index]);
            if (!check(threads[index] != NULL, "native thread start"))
            {
                return 0;
            }
        }
        fvm_port_sync_lock(gate);
        while (ready < 2)
        {
            fvm_port_sync_wait(gate);
        }
        go = 1;
        fvm_port_sync_signal_all(gate);
        fvm_port_sync_unlock(gate);
        for (index = 0; index < 2; index++)
        {
            fvm_port_thread_join(threads[index]);
        }
        if (!check((workers[0].result == DVM_STORE_OK && workers[1].result == DVM_STORE_CONFLICT) ||
                       (workers[1].result == DVM_STORE_OK && workers[0].result == DVM_STORE_CONFLICT),
                   "one CAS winner"))
        {
            return 0;
        }
        if (!check(dvm_module_store_acquire(store, image.name, &lease) == DVM_STORE_OK, "race winner acquisition") ||
            !check(dvm_module_lease_view(lease, &view) == DVM_STORE_OK, "race winner view"))
        {
            return 0;
        }
        index = workers[0].result == DVM_STORE_OK ? 0 : 1;
        if (!check(view.generation == workers[index].next && view.bytes[0] == workers[index].byte,
                   "winner bytes and generation must agree"))
        {
            return 0;
        }
        generation = view.generation;
        dvm_module_lease_release(&lease);
    }
    fvm_port_sync_destroy(gate);
    return check(dvm_module_store_destroy(&store) == DVM_STORE_OK, "race teardown");
}


/* Saturate distinct versions and reader handles, then verify complete quota recovery. */
static int bounds(void)
{
    DVM_ModuleStore *store;
    DVM_ModuleLease **leases;
    DVM_ModuleLease *extra = NULL;
    DVM_ModuleStoreStats stats;
    DVM_ModuleImage image;
    unsigned long generation = 0;
    size_t index;
    store = dvm_module_store_create(1, DVM_STORE_VERSIONS_MAX);
    leases = (DVM_ModuleLease **)calloc(DVM_STORE_LEASES_MAX, sizeof(*leases));
    if (!check(store != NULL && leases != NULL, "bounded metadata allocation"))
    {
        return 0;
    }
    image.name = "/bounded.dis";
    image.bytes = (unsigned char *)"x";
    image.size = 1;
    for (index = 0; index < DVM_STORE_VERSIONS_MAX; index++)
    {
        if (!check(dvm_module_store_publish(store, &image, DVM_STORE_ANY, &generation) == DVM_STORE_OK,
                   "admit bounded version") ||
            !check(dvm_module_store_acquire(store, image.name, &leases[index]) == DVM_STORE_OK, "pin bounded version"))
        {
            return 0;
        }
    }
    if (!check(dvm_module_store_publish(store, &image, generation, &generation) == DVM_STORE_LIMIT,
               "retired-version quota is enforced"))
    {
        return 0;
    }
    for (; index < DVM_STORE_LEASES_MAX; index++)
    {
        if (!check(dvm_module_store_acquire(store, image.name, &leases[index]) == DVM_STORE_OK, "admit bounded reader"))
        {
            return 0;
        }
    }
    if (!check(dvm_module_store_acquire(store, image.name, &extra) == DVM_STORE_LIMIT && extra == NULL,
               "reader quota preserves output"))
    {
        return 0;
    }
    for (index = 0; index < DVM_STORE_LEASES_MAX; index++)
    {
        dvm_module_lease_release(&leases[index]);
    }
    free(leases);
    dvm_module_store_stats(store, &stats);
    if (!check(stats.versions == 1 && stats.leases == 0 && stats.bytes == 1, "all retired version resources recovered"))
    {
        return 0;
    }
    return check(dvm_module_store_destroy(&store) == DVM_STORE_OK, "bounded store teardown");
}


/* Disable only LeakSanitizer's unavailable /proc inspection in this test process.
 * No parameters. Returns one; AddressSanitizer/UBSan checks remain enabled. */
int __lsan_is_turned_off(void)
{
    return 1;
}


/* Return zero only after all real namespace/OS-thread checks complete; no parameters. */
int main(void)
{
    if (!versions() || !sources() || !races() || !bounds())
    {
        return 1;
    }
    puts("PASS module store: versions, pinned quotas, providers and 200 native CAS races");
    return 0;
}
