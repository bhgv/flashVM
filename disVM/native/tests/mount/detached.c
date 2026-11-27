/* Copyright (c) 2026 bhgv
 * EN: Keep a real remote open fid alive after unbinding and destroying its namespace.
 * RU: Сохранить настоящий удалённый открытый fid после unbind и удаления namespace.
 */
#include "detached.h"
#include "export.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* EN/RU: Keep condition checks active in release builds; why is borrowed text.
 * Сохранить проверки condition в release; why — заимствованное описание. */
static void require(int condition, const char *why)
{
    if (!condition)
    {
        fprintf(stderr, "detached remote FD: %s\n", why);
        exit(2);
    }
}


/* EN: Borrow provider and create one owned remote read handle through a mounted
 * namespace. Unbind the mount and destroy that namespace before returning the
 * handle. Caller must eventually close it; failures stop this isolated test.
 * RU: Заимствовать provider и открыть один удалённый read-handle через namespace.
 * Удалить mount и namespace до возврата handle. Вызывающий обязан его закрыть;
 * ошибка завершает изолированную проверку. */
DVM_VfsHandle styxtest_detached_open(DVM_Provider *provider)
{
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_NamespacePlan target;
    DVM_ResourcePlan plan;
    DVM_VfsHandle handle = {0};
    require(group && dvm_namespace_plan(group, "/remote", &target), "mount target");
    require(dvm_namespace_mount_provider(group, &target, provider, "/remote-source") == 1, "mount provider");
    require(dvm_resource_plan(group, "/remote/first", 0, &plan), "resolve mounted file");
    require(plan.provider && dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK, "open remote fid");
    dvm_resource_close(&plan);
    require(dvm_namespace_plan(group, "/remote", &target) && dvm_namespace_unmount(group, &target, NULL) == 1,
            "unbind mounted namespace");
    require(dvm_resource_plan(group, "/remote/first", 0, &plan) && !plan.provider,
            "unbound name still resolves to provider");
    dvm_resource_close(&plan);
    dvm_namespace_release(group);
    return handle;
}

/* EN: Resolve a fresh path in group and return an owned directory cursor.
 * RU: Разрешить путь заново в group и вернуть собственный итератор каталога. */
static DVM_VfsHandle open_union(DVM_Namespace *group)
{
    DVM_ResourcePlan plan;
    DVM_VfsHandle directory = {0};
    require(dvm_resource_plan(group, "/union", 0, &plan), "resolve union");
    require(dvm_namespace_directory(&plan, &directory) == FVM_OK, "open union");
    dvm_resource_close(&plan);
    return directory;
}


/* EN: Read a fresh union listing. Verify a local-only entry and exactly the
 * expected number of occurrences of the name shared by both branches.
 * RU: Прочитать список union и сверить локальное имя и число совпадающих имён. */
static void list_union(DVM_VfsHandle *directory, unsigned first_count)
{
    unsigned first = 0;
    unsigned local = 0;
    FVM_FileInfo *entries;
    size_t count;
    do
    {
        require(dvm_vfs_list(directory, &entries, &count) == FVM_OK, "list union");
        for (size_t i = 0; i < count; i++)
        {
            first += !strcmp(entries[i].name, "first");
            local += !strcmp(entries[i].name, "local-only");
        }
        fvm_port_directory_entries_free(entries);
    } while (count);
    require(first == first_count && local == 1, "union entries after provider change");
}


/* EN/RU: See detached.h. / См. detached.h. */
void styxtest_union_loss(DVM_Provider *provider)
{
    char root[DVM_NS_PATH_LIMIT];
    DVM_Namespace *group = getcwd(root, sizeof(root)) ? dvm_namespace_create(root, "/", NULL) : NULL;
    require(group && dvm_namespace_pin(group) &&
        dvm_stack_mount(group, provider, "/union", DVM_NS_BEFORE, 0) == 1, "mount union head");

    DVM_VfsHandle directory = open_union(group);
    list_union(&directory, 2);
    DVM_ResourcePlan plan;
    require(dvm_resource_plan(group, "/union/first", 0, &plan) &&
        dvm_union_select(&plan, 0) == FVM_OK && plan.provider, "select remote head");
    DVM_VfsHandle held = {0};
    require(dvm_provider_open(plan.provider, plan.relative, "r", &held) == FVM_OK, "open remote head");
    dvm_resource_close(&plan);
    char bytes[16];
    DVM_IoResult before = dvm_vfs_read_chunk(&held, bytes, sizeof(bytes));
    require(before.status == FVM_OK && before.count == 7 && !memcmp(bytes, "initial", 7),
        "read remote head");

    DVM_VfsHandle control = {0};
    require(dvm_provider_open(provider, "ctl", "wo", &control) == FVM_OK, "open loss marker");
    require(dvm_vfs_write_chunk(&control, "change", 6).status != FVM_OK, "relay did not disconnect");
    require(dvm_vfs_read_chunk(&held, bytes, sizeof(bytes)).status != FVM_OK,
        "held file changed identity on disconnect");

    require(dvm_resource_plan(group, "/union/first", 0, &plan) &&
        dvm_union_select(&plan, 0) == FVM_OK && !plan.provider && plan.anchor, "select native fallback");
    FVM_PortFile *local = NULL;
    FVM_PortDirectory *unused = NULL;
    require(dvm_resource_open(&plan, "r", &local, &unused) == FVM_OK, "open native fallback");
    dvm_resource_close(&plan);
    DVM_VfsHandle file = {0};
    require(dvm_vfs_adopt(&file, local, unused), "adopt native fallback");
    DVM_IoResult read = dvm_vfs_read_chunk(&file, bytes, sizeof(bytes));
    require(read.status == FVM_OK && read.count == 8 && !memcmp(bytes, "fallback", 8), "read fallback");

    long position = -1;
    require(dvm_vfs_seek(&directory, 0, FVM_FILE_SEEK_START, &position) == FVM_OK &&
        position == 0, "rewind after loss");
    list_union(&directory, 1);
    DVM_VfsHandle fresh = open_union(group);
    list_union(&fresh, 1);

    dvm_vfs_close(&fresh);
    dvm_vfs_close(&file);
    dvm_vfs_close(&held);
    dvm_vfs_close(&directory);
    dvm_vfs_close(&control);
    dvm_namespace_release(group);
    dvm_provider_release(provider);
}
