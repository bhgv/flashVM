/* Copyright (c) 2026 bhgv
 * EN: Pre-init root union, identities, isolation, strict absence fallback and per-file reorder.
 * RU: Pre-init root union, identity, изоляция, absence-only fallback и перестановка файла.
 */
#include "backend.h"


/* EN/RU: Resolve live file in borrowed group, optionally return copied source role; close plan.
 * Разрешить live file в borrowed group, optional copied role, закрыть plan. */
static char value(DVM_Namespace *group, const char *path, unsigned *role)
{
    DVM_ResourcePlan plan;
    FVM_FileInfo info;
    DVM_VfsHandle handle = {0};
    char byte;
    size_t count;
    launch_check(dvm_resource_plan(group, path, 0, &plan) && dvm_resource_info(&plan, &info) == FVM_OK &&
        dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK &&
        dvm_vfs_read(&handle, &byte, 1, &count) == FVM_OK && count == 1, "ordinary live file");
    if (role)
    {
        *role = plan.origin.role;
    }
    dvm_vfs_close(&handle);
    dvm_resource_close(&plan);
    return byte;
}


/* EN/RU: Verify rejected policy/identity/directory/cancellation leaves no namespace or callbacks.
 * Проверить refusal policy/identity/type/cancel без namespace publication и лишних callbacks. */
static void refusals(DVM_LaunchOptions *options, LaunchState *terminal, LaunchState *provider)
{
    options->policy = 2;
    launch_check(!dvm_launch_prepare(options), "unknown policy accepted");
    options->policy = DVM_LAUNCH_TERMINAL_FIRST;
    options->terminal_origin.system[42] = 'B';
    launch_check(!dvm_launch_prepare(options), "noncanonical identity accepted");
    options->terminal_origin.system[42] = 'A';
    options->provider_origin.session[0] = 'X';
    launch_check(!dvm_launch_prepare(options), "session mismatch accepted");
    options->provider_origin.session[0] = 'S';
    terminal->file_root = 1;
    launch_check(!dvm_launch_prepare(options), "file root accepted");
    terminal->file_root = 0;
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "cancel scope");
    unsigned before = terminal->roots+provider->roots;
    dvm_cancel_request(cancel);
    launch_check(!dvm_launch_prepare(options) && terminal->roots+provider->roots == before,
                 "pre-cancel invoked root callback");
    launch_check(dvm_cancel_leave(&scope), "cancel scope retirement");
    dvm_cancel_release(cancel);
    cancel = dvm_cancel_create();
    terminal->cancel = cancel;
    terminal->cancel_root = 1;
    launch_check(cancel && dvm_cancel_enter(&scope, cancel), "second cancel scope");
    before = provider->roots;
    launch_check(!dvm_launch_prepare(options) && provider->roots == before, "root callback continued after cancel");
    launch_check(dvm_cancel_leave(&scope), "second scope retirement");
    terminal->cancel_root = 0;
    dvm_cancel_release(cancel);
}


/* EN/RU: Exercise actual namespace plans/bind/move and held streams; every owner retires.
 * Проверить настоящие plans/bind/move и held streams; освободить все owners. */
int main(void)
{
    LaunchState terminal = {0}, provider = {0};
    terminal.value = 'T';
    terminal.terminal = 1;
    provider.value = 'P';
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    refusals(&options, &terminal, &provider);
    DVM_Namespace *group = dvm_launch_prepare(&options);
    DVM_Namespace *other = dvm_launch_prepare(&options);
    launch_check(group && other && !group->root_anchor && group->absence_only, "private isolated environments");
    unsigned role;
    launch_check(value(group, "/dev/camera", &role) == 'T' && role == DVM_ORIGIN_TERMINAL &&
                 value(group, "/server", &role) == 'P' && role == DVM_ORIGIN_PROVIDER, "terminal-first fallback role");
    DVM_ResourcePlan denied;
    FVM_FileInfo info;
    unsigned before = provider.denied;
    launch_check(dvm_resource_plan(group, "/denied", 0, &denied) &&
        dvm_resource_info(&denied, &info) != FVM_OK && provider.denied == before, "denial fell through");
    dvm_resource_close(&denied);
    launch_check(dvm_resource_plan(group, "/etc/passwd", 0, &denied) &&
        dvm_resource_info(&denied, &info) != FVM_OK, "native host fallback escaped roots");
    dvm_resource_close(&denied);
    DVM_Namespace *copy = dvm_namespace_copy(group);
    DVM_ResourcePlan camera;
    DVM_VfsHandle held = {0};
    launch_check(copy && dvm_resource_plan(group, "/dev/camera", 0, &camera) && camera.layers &&
        camera.origin.role == DVM_ORIGIN_TERMINAL && camera.layers->origin.role == DVM_ORIGIN_PROVIDER &&
        dvm_provider_open(camera.provider, camera.relative, "r", &held) == FVM_OK, "effective source stack");
    launch_check(dvm_stack_move(&camera, camera.names.version, camera.layer_id, -1) == 1, "child file move");
    dvm_resource_close(&camera);
    launch_check(value(group, "/dev/camera", &role) == 'P' && role == DVM_ORIGIN_PROVIDER &&
        value(group, "/dev/keyboard", &role) == 'T' && value(copy, "/dev/camera", &role) == 'T' &&
        value(other, "/dev/camera", &role) == 'T', "move changed siblings or other namespace");
    char byte;
    size_t count;
    terminal.value = 'C';
    launch_check(dvm_vfs_read(&held, &byte, 1, &count) == FVM_OK && count == 1 && byte == 'C', "held FD lost live source");
    dvm_vfs_close(&held);
    DVM_ResourcePlan target;
    launch_check(dvm_resource_plan(group, "/dev/camera", 0, &camera) &&
        dvm_resource_plan(group, "/saved", 0, &target) && dvm_stack_bind(&camera, &target, 0, 0) == 1,
        "file bind preserves provenance");
    dvm_resource_close(&target);
    dvm_resource_close(&camera);
    launch_check(value(group, "/saved", &role) == 'P' && role == DVM_ORIGIN_PROVIDER, "bound source association");
    DVM_NamespacePlan root;
    launch_check(dvm_namespace_plan(group, "/", &root) && dvm_namespace_unmount(group, &root, NULL) == 1,
                 "remove granted root stack");
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    launch_check(dvm_resource_plan(group, "/etc/passwd", 0, &denied) &&
        dvm_resource_raw_info(&denied, &info) == FVM_NOT_FOUND &&
        dvm_resource_open(&denied, "r", &file, &directory) == FVM_NOT_FOUND && !file && !directory,
        "removing roots exposed implicit host paths");
    dvm_resource_close(&denied);
    dvm_namespace_release(copy);
    dvm_namespace_release(other);
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "root/stream lifetime leak");
    puts("PASS launch preparation origins isolation live roots and file moves");
    return 0;
}
