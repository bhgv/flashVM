/* Copyright (c) 2026 bhgv
 * EN: Real remote directory/file capture through SDK bind, indexed stacks and prepared views.
 * RU: Настоящий remote directory/file capture через SDK bind, indexed stacks и prepared views.
 */
#include "mount.h"
#include "export.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Concrete failure remains active in every native compiler configuration.
 * Конкретный failure остаётся активным во всех native compiler configurations. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "remote subroot: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Open through retained namespace plans; ordinary native/provider routes share handle APIs.
 * Открыть через retained namespace plans; native/provider routes используют общий handle API. */
static DVM_VfsHandle openfile(DVM_Namespace *group, const char *path)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle fd = {0};
    check(dvm_resource_plan(group, path, 0, &plan) && dvm_union_select(&plan, 0) == FVM_OK, "file selection");
    FVM_Result result;
    if (plan.provider)
    {
        result = dvm_provider_open(plan.provider, plan.relative, "r", &fd);
    }
    else
    {
        FVM_PortFile *file = NULL;
        FVM_PortDirectory *directory = NULL;
        result = dvm_resource_open(&plan, "r", &file, &directory);
        if (result == FVM_OK)
        {
            check(dvm_vfs_adopt(&fd, file, directory), "native adoption");
        }
    }
    dvm_resource_close(&plan);
    check(result == FVM_OK, "file open");
    return fd;
}


/* EN/RU: One precise byte from the currently selected object, then close its separate cursor.
 * Один precise byte из выбранного объекта, затем close отдельного cursor. */
static char value(DVM_Namespace *group, const char *path)
{
    DVM_VfsHandle fd = openfile(group, path);
    char out = 0;
    DVM_IoResult result = dvm_vfs_read_chunk(&fd, &out, 1);
    check(result.status == FVM_OK && result.count == 1, path);
    dvm_vfs_close(&fd);
    return out;
}


/* EN/RU: Send one fixture control command through the same original Styx connection.
 * Послать fixture control command через то же original Styx connection. */
static void command(StyxClient *client, const char *text)
{
    StyxFid *fid = styxclientwalk(client, "ctl");
    check(fid && styxclientopen(fid, 1), "control open");
    StyxTransfer result;
    styxclientwrite(fid, 0, text, strlen(text), &result);
    check(result.success && result.count == strlen(text), "control write");
    styxclientclunk(fid);
}


/* EN/RU: Bind source at a captured namespace revision; source may require native remote node capture.
 * Bind source по captured namespace revision; source может требовать remote node capture. */
static void bindroot(DVM_Namespace *group, const char *source, const char *target, unsigned flags, int64_t index)
{
    DVM_ResourcePlan a, b;
    check(dvm_resource_plan(group, source, 0, &a) && dvm_resource_plan(group, target, 0, &b), "bind plans");
    check(dvm_stack_bind(&a, &b, flags, index) == 1, "captured stack bind");
    dvm_resource_close(&b);
    dvm_resource_close(&a);
}


/* EN/RU: Enumerate a live captured directory with its unrelated native union sibling.
 * Перечислить live captured directory с unrelated native union sibling. */
static void listing(DVM_Namespace *group, int item, int added)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle fd = {0};
    check(dvm_resource_plan(group, "/alias", 0, &plan) && dvm_namespace_directory(&plan, &fd) == FVM_OK, "directory open");
    dvm_resource_close(&plan);
    unsigned items = 0, additions = 0, locals = 0;
    for (unsigned i = 0; i < 32; i++)
    {
        FVM_FileInfo *entries = NULL;
        size_t count = 0;
        check(dvm_vfs_list(&fd, &entries, &count) == FVM_OK, "live directory list");
        for (size_t j = 0; j < count; j++)
        {
            items += !strcmp(entries[j].name, "item");
            additions += !strcmp(entries[j].name, "added");
            locals += !strcmp(entries[j].name, "local");
        }
        fvm_port_directory_entries_free(entries);
        if (!count)
        {
            break;
        }
        check(i < 31, "directory enumeration bound");
    }
    check(items == (unsigned)item && additions == (unsigned)added && locals == 1, "captured tree froze/lost union sibling");
    dvm_vfs_close(&fd);
}


/* EN/RU: Prepared-tree policy remains live after root capture and original source replacement.
 * Prepared-tree policy остаётся live после root capture и замены original source. */
static int policy(void *state, const char *path, unsigned access)
{
    (void)path;
    (void)access;
    return *(int *)state;
}


/* EN/RU: Exercise real remote pinning, live membership, ACLs, events and native lifetime.
 * Проверить real remote pinning, live membership, ACL, events и native lifetime. */
int main(int argc, char **argv)
{
    check(argc == 4, "port/root/scenario");
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(socketfd >= 0 && !connect(socketfd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(socketfd, 2048, "fixture", "", 2000, 128);
    check(client && styxclientretain(client), "client");
    DVM_Provider *raw = styxmountprovider(client, FVM_ACCESS_READ|FVM_ACCESS_EXEC);
    check(raw && !dvm_provider_bindable(raw) && dvm_provider_capturable(raw), "directory capture capability");
    DVM_Changes *source = dvm_provider_changes(raw);
    check(changesretain(source), "source journal pin");
    DVM_Namespace *group = dvm_namespace_create(argv[2], "/", NULL);
    DVM_NamespacePlan target;
    check(group && dvm_namespace_pin(group) && dvm_namespace_plan(group, "/remote", &target) &&
          dvm_namespace_mount_provider(group, &target, raw, "/remote-styx") == 1, "initial remote mount");
    check(styxclientretain(client) && !styxmountproviderat(client, "../source", 5) && styxclientalive(client), "invalid subroot setup");
    StyxFid *base = styxclientwalk(client, "source/item");
    check(base && !styxclientwalkfrom(base, "child") && !styxclientwalkfrom(base, "../item"), "file base confinement");
    StyxFid *clone = styxclientwalkfrom(base, ".");
    styxclientclunk(base);
    check(clone && styxclientopen(clone, 0) && !styxclientwalkfrom(clone, "."), "unopened base/zero clone");
    styxclientclunk(clone);
    if (!strcmp(argv[3], "at"))
    {
        check(styxclientretain(client), "subroot owner");
        DVM_Provider *node = styxmountproviderat(client, "source", 5);
        check(node && dvm_stack_mount(group, node, "/alias", DVM_NS_BEFORE, 0) == 1, "explicit rooted mount");
        dvm_changes_destroy(source);
        source = dvm_provider_changes(node);
        check(changesretain(source), "independent rooted source journal");
        dvm_provider_release(node);
    }
    else
    {
        bindroot(group, "/remote/source", "/alias", DVM_NS_BEFORE, 0);
    }
    bindroot(group, "/alias/sub", "/nested", 0, 0);
    bindroot(group, "/alias/item", "/files/saved", 0, 0);
    int allowed = 1;
    DVM_View *builder = dvm_view_create(policy, &allowed, NULL);
    check(builder && dvm_view_map(builder, "/", raw, "source", 5, DVM_VIEW_TREE) &&
          dvm_view_map(builder, "/private", NULL, ".", 0, 0) &&
          dvm_view_map(builder, "/plug", raw, "source/item", 4, 0), "prepared composition");
    DVM_Provider *view = dvm_view_freeze(builder);
    check(view && dvm_stack_mount(group, view, "/prepared", 0, 0) == 1, "prepared captured root");
    DVM_ResourcePlan secret;
    FVM_FileInfo hiddenmeta;
    check(dvm_resource_plan(group, "/prepared/private", 0, &secret) &&
          dvm_resource_info(&secret, &hiddenmeta) != FVM_OK, "prepared hidden node escaped capture");
    dvm_resource_close(&secret);
    DVM_ResourcePlan root;
    check(dvm_resource_plan(group, "/alias", 0, &root), "captured journal lookup");
    DVM_Changes *journal = dvm_provider_changes(root.provider);
    check(changesretain(journal), "captured journal pin");
    dvm_resource_close(&root);
    listing(group, 1, 0);
    command(client, "replace");
    check(value(group, "/remote/source/item") == 'B' && value(group, "/alias/item") == 'A' &&
          value(group, "/prepared/item") == 'A' && value(group, "/prepared/plug") == 'B', "bind reopened replaced source pathname");
    command(client, "add");
    listing(group, 1, 1);
    check(value(group, "/alias/added") == 'V', "captured root froze child set");
    command(client, "swap-sub");
    check(value(group, "/alias/sub/item") == 'M' && value(group, "/nested/item") == 'N', "nested fid redirected after child replacement");
    DVM_ResourcePlan selected;
    check(dvm_resource_plan(group, "/nested", 0, &selected) && dvm_namespace_plan(group, "/lowlevel", &target) &&
          dvm_namespace_bind_provider(group, &selected, &target) == 1, "low-level captured provider bind");
    dvm_resource_close(&selected);
    check(value(group, "/lowlevel/item") == 'N', "low-level bind lost anchored dot");
    command(client, "hide");
    listing(group, 0, 1);
    check(value(group, "/files/saved") == 'A', "held file lost identity after unlink");
    DVM_VfsHandle held = openfile(group, "/files/saved");
    command(client, "deny");
    char byte;
    DVM_IoResult denied = dvm_vfs_read_chunk(&held, &byte, 1);
    check(denied.status != FVM_OK && !denied.count, "captured open bypassed operation-time revocation");
    dvm_vfs_close(&held);
    command(client, "restore");
    check(value(group, "/files/saved") == 'A', "rights restoration did not permit new open");
    allowed = 0;
    check(dvm_resource_plan(group, "/prepared/item", 0, &selected), "policy plan");
    FVM_FileInfo meta;
    check(dvm_resource_info(&selected, &meta) != FVM_OK, "captured prepared policy revoked incorrectly");
    dvm_resource_close(&selected);
    allowed = 1;
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
          dvm_changes_ack(journal, &cursor, &batch) == FVM_OK, "captured initial events");
    dvm_provider_release(raw);
    dvm_provider_release(view);
    check(changespost(source, DVM_CHANGE_CONTENT|DVM_CHANGE_NAMESPACE, DVM_CHANGE_RESOURCE, 12) &&
          dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
          batch.event.kinds == (DVM_CHANGE_CONTENT|DVM_CHANGE_NAMESPACE) &&
          (journal == source ? batch.event.flags == DVM_CHANGE_RESOURCE && batch.event.resource == 12 : !batch.event.flags),
          "captured roots lost source invalidation/lifetime");
    check(value(group, "/nested/item") == 'N', "source owner loss invalidated nested root");
    bindroot(group, "/remote/source", "/alias", DVM_NS_INSERT, 1);
    check(value(group, "/alias/item") == 'A', "directory MINS changed top layer");
    DVM_Namespace *copy = dvm_namespace_copy(group);
    check(copy && dvm_resource_plan(copy, "/alias", 0, &selected) &&
          dvm_stack_move(&selected, selected.names.version, selected.layer_id, -1) == 1, "copied captured directory move");
    dvm_resource_close(&selected);
    check(value(copy, "/alias/item") == 'B' && value(group, "/alias/item") == 'A', "captured directory order leaked across namespace copy");
    dvm_namespace_release(copy);
    check(dvm_resource_plan(group, "/alias", 0, &selected) &&
          dvm_stack_move(&selected, selected.names.version, selected.layer_id, -1) == 1, "captured directory reorder");
    dvm_resource_close(&selected);
    check(value(group, "/alias/item") == 'B' && value(group, "/alias/local") == 'L', "directory move lost layers/sibling");
    check(dvm_namespace_plan(group, "/alias", &target) && dvm_namespace_unmount(group, &target, NULL) == 1 &&
          value(group, "/alias/local") == 'L' && value(group, "/nested/item") == 'N', "captured directory unmount redirected other roots");
    dvm_changes_destroy(journal);
    dvm_changes_destroy(source);
    dvm_namespace_release(group);
    command(client, "stop");
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS remote subroot capture/live tree/policy/source events: %s\n", argv[3]);
    return 0;
}
