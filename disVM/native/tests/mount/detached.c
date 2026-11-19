/* Copyright (c) 2026 bhgv
 * EN: Keep a real remote open fid alive after unbinding and destroying its namespace.
 * RU: Сохранить настоящий удалённый открытый fid после unbind и удаления namespace.
 */
#include "mount.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>


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
