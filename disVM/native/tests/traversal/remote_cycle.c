/* Copyright (c) 2026 bhgv
 * EN: Two real libstyx servers recurse across fragmented PHY links; distinct local mounts share one logical export endpoint.
 * RU: Два настоящих libstyx server рекурсируют через fragmented PHY; разные local mounts разделяют logical export endpoint.
 */
#define _POSIX_C_SOURCE 200809L
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "mount.h"
#include "drivers/posix/fd.h"
#include "../phy/limited.h"
#include "../../../runtime/vfs/export.h"
#include "../../../runtime/vfs/internal.h"
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>


/* EN/RU: Test owns each server/peer until native owner joins; peer changes occur only between completed RPCs.
 * Test владеет server/peer до join; peer changes лишь между completed RPCs.
 */
typedef struct Endpoint
{
    Styxserver server; /* Sole native server owner accesses its original core. */
    RemotePhy *phy; /* Moved to server initialization on the native owner. */
    WalkEntity root; /* Immutable virtual export identity, independent of local mount allocation. */
    DVM_Provider *peer; /* Borrowed reverse mount; main retains it through all RPCs. */
    unsigned calls; /* Atomic actual file metadata callbacks, never inferred from elapsed time. */
    int deny; /* Atomic explicit denial after observing an inner cycle; must remain an opaque refusal. */
    int forward; /* Atomic whether this fixture forwards the file metadata lookup. */
} Endpoint;
static __thread Endpoint *owner; /* Callback-local native owner, never another guest or shared scratch. */
static pthread_barrier_t ready; /* Admission checkpoint after both actual servers initialize. */


/* EN/RU: Explicit release-build contract failure with borrowed diagnostic, no assert.
 * Явный contract failure независимо от build; diagnostic borrowed, без assert.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprint(2, "remote walk: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Root metadata is local; the actual file lookup forwards into the opposite real Styx mount.
 * Metadata root локальна; actual file lookup направляется в opposite Styx mount.
 */
static char *metadata(Qid qid, Dir *output)
{
    if (qid.path == 1)
    {
        __atomic_add_fetch(&owner->calls, 1, __ATOMIC_RELAXED);
        if (__atomic_load_n(&owner->forward, __ATOMIC_ACQUIRE))
        {
            FVM_FileInfo info;
            FVM_Result result = dvm_provider_info(owner->peer, "loop", &info);
            if (result != FVM_OK)
            {
                return __atomic_load_n(&owner->deny, __ATOMIC_ACQUIRE) ? Eperm :
                    result == DVM_VFS_CYCLE ? styxphycycleerror(&owner->server) : "recursive namespace lookup";
            }
        }
    }
    Styxfile *file = styxfindfile(&owner->server, qid.path);
    if (!file)
    {
        return Enonexist;
    }
    *output = file->d;
    return nil;
}


/* EN/RU: Own the unchanged core on one dedicated native thread; export root never changes mid-connection.
 * Владеть unchanged core на dedicated native thread; export root не меняется посреди connection.
 */
static void *serve(void *state)
{
    owner = state;
    Styxops operations = {0};
    operations.stat = metadata;
    check(!styxphywalkinit(&owner->server, &operations, owner->phy, 0555, 1, 4, &owner->root), "server admission");
    check(styxaddfile(&owner->server, Qroot, 1, "loop", 0444, "fixture") != NULL, "served node");
    pthread_barrier_wait(&ready);
    while (!styxwait(&owner->server) && !styxprocess(&owner->server))
    {
        check(!walk_depth(), "server owner retained completed remote history");
    }
    check(!styxend(&owner->server) && !walk_depth(), "server retirement");
    return NULL;
}


/* EN/RU: Independent healthy branch supplies the same union name; no socket or device specialization.
 * Independent healthy branch предоставляет то же union name, без socket/device specialization.
 */
static FVM_Result healthy_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "loop"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = 0555;
    return FVM_OK;
}


/* EN/RU: Fixture opens are unused; refuse without publishing output ownership.
 * Fixture opens не используются; отказ без publication output ownership.
 */
static FVM_Result healthy_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    return FVM_UNSUPPORTED;
}


/* EN/RU: Healthy provider has no owned callback state; release has no effects.
 * Healthy provider не владеет callback state; release без effects.
 */
static void healthy_destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps healthy_ops = {healthy_info, healthy_open, healthy_destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Local lower union branch. */


/* EN/RU: Exercise wire-preserved A→B→A detection before re-enqueuing onto blocked A, healthy fallback and later recovery.
 * Проверить wire A→B→A до повторной постановки в blocked A, healthy fallback и последующий recovery.
 */
int main(void)
{
    unsigned long domain[4] = {10, 20, 30, 40};
    check(walk_domain(domain), "domain setup");
    Endpoint a = {0};
    Endpoint b = {0};
    check(walk_entity(&a.root) && walk_entity(&b.root), "distinct export endpoints");
    int first[2];
    int second[2];
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, first) && !socketpair(AF_UNIX, SOCK_STREAM, 0, second), "physical test carriers");
    unsigned capabilities = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    a.phy = phyfixture(phyposixfd(first[0], capabilities), 0);
    b.phy = phyfixture(phyposixfd(second[0], capabilities), 0);
    check(a.phy && b.phy && !pthread_barrier_init(&ready, NULL, 3), "bounded PHY/init barrier");
    pthread_t left;
    pthread_t right;
    check(!pthread_create(&left, NULL, serve, &a) && !pthread_create(&right, NULL, serve, &b), "native server owners");
    pthread_barrier_wait(&ready);
    StyxClient *client = styxclientwalkmountphy(phyfixture(phyposixfd(first[1], capabilities), 0), 256, "fixture", "", 2000, 8, 55);
    check(client && styxclientretain(client), "first original client and alias pin");
    DVM_Provider *first_mount = styxmountprovider(client, FVM_ACCESS_READ | FVM_ACCESS_EXEC);
    DVM_Provider *alias_mount = styxmountprovider(client, FVM_ACCESS_READ | FVM_ACCESS_EXEC);
    DVM_Provider *other = styxmountprovider(styxclientwalkmountphy(phyfixture(phyposixfd(second[1], capabilities), 0),
        256, "fixture", "", 2000, 8, 55), FVM_ACCESS_READ | FVM_ACCESS_EXEC);
    check(first_mount && alias_mount && other, "actual native mount factories");
    a.peer = other;
    b.peer = alias_mount; /* A different local provider identity must still detect A's logical endpoint. */
    __atomic_store_n(&a.forward, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&b.forward, 1, __ATOMIC_RELEASE);
    FVM_FileInfo info;
    check(dvm_provider_info(first_mount, "loop", &info) == DVM_VFS_CYCLE &&
        a.calls == 1 && b.calls == 1 && !walk_depth(), "A-B-A loop was lost, repeated callback or stalled");
    __atomic_store_n(&b.deny, 1, __ATOMIC_RELEASE);
    check(dvm_provider_info(first_mount, "loop", &info) == FVM_ERROR && a.calls == 2 && b.calls == 2,
        "skipped inner cycle relabeled an explicit permission denial");
    __atomic_store_n(&b.deny, 0, __ATOMIC_RELEASE);
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_Provider *healthy = dvm_provider_create(&healthy_ops, NULL);
    check(group && healthy && dvm_stack_mount(group, first_mount, "/", 0, 0) == 1 &&
        dvm_stack_mount(group, healthy, "/", DVM_NS_AFTER, 0) == 1, "real union composition");
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(group, "/loop", 0, &plan) && dvm_resource_info(&plan, &info) == FVM_OK &&
        plan.provider == healthy && a.calls == 3 && b.calls == 3 && !walk_depth(), "wire cycle hid healthy lower union branch");
    dvm_resource_close(&plan);
    __atomic_store_n(&b.forward, 0, __ATOMIC_RELEASE);
    check(dvm_provider_info(first_mount, "loop", &info) == FVM_OK && a.calls == 4 && b.calls == 4 && !walk_depth(),
        "completed history poisoned later noncyclic traversal");
    dvm_namespace_release(group);
    dvm_provider_release(healthy);
    dvm_provider_release(first_mount);
    dvm_provider_release(alias_mount);
    dvm_provider_release(other);
    check(!pthread_join(left, NULL) && !pthread_join(right, NULL), "native server retirement joins");
    pthread_barrier_destroy(&ready);
    puts("PASS actual original Styx A-B-A cycle, distinct mount aliases, lower union fallback and recovery");
    return 0;
}
