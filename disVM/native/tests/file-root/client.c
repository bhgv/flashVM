/* Copyright (c) 2026 bhgv
 * EN: Actual file-root Styx mount/bind into populated native directories with indexed leaf stacks.
 * RU: Настоящий file-root Styx mount/bind в заполненные native directories с indexed leaf stacks.
 */
#include "mount.h"
#include "export.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Keep explicit native checks active in every build configuration.
 * Сохранить явные native checks во всех build configurations. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "file-root mount: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Open one retained selected leaf; native and provider branches share ordinary handle APIs.
 * Открыть retained selected leaf; native/provider branches используют ordinary handle API. */
static DVM_VfsHandle openleaf(DVM_Namespace *group, const char *path, const char *mode)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle out = {0};
    check(dvm_resource_plan(group, path, 0, &plan) && dvm_union_select(&plan, 0) == FVM_OK, "leaf resolution");
    FVM_Result status;
    if (plan.provider)
    {
        status = dvm_provider_open(plan.provider, plan.relative, mode, &out);
    }
    else
    {
        FVM_PortFile *file = NULL;
        FVM_PortDirectory *directory = NULL;
        status = dvm_resource_open(&plan, mode, &file, &directory);
        if (status == FVM_OK)
        {
            check(dvm_vfs_adopt(&out, file, directory), "native handle adoption");
        }
    }
    dvm_resource_close(&plan);
    check(status == FVM_OK, "ordinary leaf open");
    return out;
}


/* EN/RU: Read one confirmed byte and close its independently pinned handle.
 * Прочитать один confirmed byte и закрыть independently pinned handle. */
static char byte(DVM_Namespace *group, const char *path)
{
    DVM_VfsHandle fd = openleaf(group, path, "r");
    char value = 0;
    DVM_IoResult result = dvm_vfs_read_chunk(&fd, &value, 1);
    check(result.status == FVM_OK && result.count == 1, "precise read");
    dvm_vfs_close(&fd);
    return value;
}


/* EN/RU: List exact graft names once beside unrelated real files; no source metadata name leakage.
 * Перечислить graft names ровно один раз рядом с real files; без утечки source metadata name. */
static void listing(DVM_Namespace *group, int imported, int copied)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle directory = {0};
    check(dvm_resource_plan(group, "/dst", 0, &plan) &&
          dvm_namespace_directory(&plan, &directory) == FVM_OK, "directory projection open");
    dvm_resource_close(&plan);
    unsigned have_import = 0, have_copy = 0, have_native = 0, have_collision = 0;
    for (unsigned i = 0; i < 256; i++)
    {
        FVM_FileInfo *entries = NULL;
        size_t count = 0;
        check(dvm_vfs_list(&directory, &entries, &count) == FVM_OK, "directory listing");
        for (size_t j = 0; j < count; j++)
        {
            have_import += !strcmp(entries[j].name, "imported");
            have_copy += !strcmp(entries[j].name, "copy");
            have_native += !strcmp(entries[j].name, "neighbour");
            have_collision += !strcmp(entries[j].name, "collision");
        }
        fvm_port_directory_entries_free(entries);
        if (!count)
        {
            break;
        }
        check(i < 255, "directory failed to reach EOF");
    }
    check(have_import == (unsigned)imported && have_copy == (unsigned)copied &&
          have_native == 1 && have_collision == 1, "missing/duplicate graft or hidden sibling");
    dvm_vfs_close(&directory);
}


/* EN/RU: Commit plain bind of retained source stack to an explicit new leaf; source remains borrowed.
 * Применить plain bind retained source stack в explicit new leaf; source borrowed. */
static void bindleaf(DVM_Namespace *group, const char *source, const char *target, unsigned flags, int64_t index)
{
    DVM_ResourcePlan a, b;
    check(dvm_resource_plan(group, source, 0, &a) && dvm_resource_plan(group, target, 0, &b), "bind plans");
    check(dvm_stack_bind(&a, &b, flags, index) == 1, "plain leaf bind");
    dvm_resource_close(&b);
    dvm_resource_close(&a);
}


/* EN/RU: Remove the exact leaf's overlay; parent and sibling bindings remain untouched.
 * Удалить overlay exact leaf; parent и sibling bindings не меняются. */
static void unmountleaf(DVM_Namespace *group, const char *path)
{
    DVM_NamespacePlan plan;
    check(dvm_namespace_plan(group, path, &plan) && dvm_namespace_unmount(group, &plan, NULL) == 1, "leaf unmount");
}


/* EN/RU: End-to-end actual file Rattach, native SDK mount/bind, enumeration, mutation and cleanup.
 * End-to-end actual file Rattach, native SDK mount/bind, listing, mutation и cleanup. */
int main(int argc, char **argv)
{
    check(argc == 4, "port/native root/scenario");
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    check(fd >= 0 && !connect(fd, (struct sockaddr *)&address, sizeof(address)), "connect");
    StyxClient *client = styxclientmountfd(fd, 1024, "fixture", "file", 2000, 8);
    check(client && !styxclientrootdirectory(client) && styxclientretain(client), "file root attach");
    DVM_Provider *provider = styxmountprovider(client, FVM_ACCESS_MASK);
    FVM_FileInfo metadata;
    check(provider && dvm_provider_bindable(provider) && dvm_provider_info(provider, ".", &metadata) == FVM_OK &&
          !metadata.directory && dvm_provider_info(provider, "hidden", &metadata) != FVM_OK, "true single-file root/type confinement");
    DVM_Namespace *group = dvm_namespace_create(argv[2], "/", NULL);
    check(group && dvm_namespace_pin(group), "populated native namespace");
    check(dvm_stack_mount(group, provider, "/dst/imported", DVM_NS_BEFORE, 0) == 1, "mount missing leaf");
    check(byte(group, "/dst/imported") == 'Q' && byte(group, "/dst/neighbour") == 'N', "mount covered parent/sibling");
    listing(group, 1, 0);
    bindleaf(group, "/dst/imported", "/dst/copy", DVM_NS_AFTER, 0);
    check(byte(group, "/dst/copy") == 'Q', "plain bind of mounted file");
    check(dvm_stack_mount(group, provider, "/dst/collision", DVM_NS_AFTER, 0) == 1 &&
          byte(group, "/dst/collision") == 'C', "AFTER leaf order");
    check(dvm_stack_mount(group, provider, "/dst/collision", DVM_NS_INSERT, -3) == 1 &&
          byte(group, "/dst/collision") == 'Q', "MINS signed leaf order");
    listing(group, 1, 1);
    DVM_ResourcePlan plan;
    check(dvm_resource_plan(group, "/dst/collision", 0, &plan), "move leaf plan");
    check(dvm_stack_move(&plan, plan.names.version, plan.layer_id, -1) == 1, "move only file layer");
    dvm_resource_close(&plan);
    check(byte(group, "/dst/collision") == 'C' && byte(group, "/dst/neighbour") == 'N', "move affected directory/sibling");
    check(dvm_stack_mount(group, provider, "/missing/leaf", 0, 0) == -1 &&
          dvm_stack_mount(group, provider, "/dst", 0, 0) == -1, "missing parent or directory replacement accepted");
    DVM_VfsHandle held = openleaf(group, "/dst/imported", "r");
    DVM_VfsHandle writer = openleaf(group, "/dst/copy", "wo");
    DVM_IoResult written = dvm_vfs_write_chunk(&writer, "Z", 1);
    check(written.status == FVM_OK && written.count == 1, "single-file root write");
    dvm_vfs_close(&writer);
    check(byte(group, "/dst/imported") == 'Z', "live file aliases froze content");
    unmountleaf(group, "/dst/imported");
    unmountleaf(group, "/dst/collision");
    listing(group, 0, 1);
    check(byte(group, "/dst/collision") == 'C' && byte(group, "/dst/neighbour") == 'N', "unmount removed native siblings");
    char value;
    DVM_IoResult read = dvm_vfs_read_chunk(&held, &value, 1);
    check(read.status == FVM_OK && read.count == 1 && value == 'Z', "open fid lost lifetime after unmount");
    dvm_vfs_close(&held);
    dvm_namespace_release(group);
    dvm_provider_release(provider);
    styxclientshutdown(client);
    styxclientfree(client);
    printf("PASS single-file mount/bind and populated directory: %s\n", argv[3]);
    return 0;
}
