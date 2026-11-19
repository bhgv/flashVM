/* Copyright (c) 2026 bhgv
 * EN: Real native provider recursion and union fallthrough; owner-first launch composition retains all original capabilities.
 * RU: Настоящая native provider recursion и union fallthrough; owner-first launch сохраняет original capabilities.
 */
#include "../launch/backend.h"
#include "../../../runtime/vfs/export.h"


/* EN/RU: Borrowed recursion edges never add native reference cycles; main keeps both endpoint factories alive.
 * Borrowed recursion edges не создают native reference cycles; main удерживает обе factories.
 */
typedef struct Cycle
{
    DVM_Provider *next; /* Borrowed synchronous recursion destination. */
    unsigned calls; /* Actual metadata callback entries; repeated state must stop before callback. */
} Cycle;


/* EN/RU: Ordinary dot metadata validates a mount; other names recurse through the actual public provider dispatcher.
 * Dot metadata валидирует mount; прочие имена идут через actual public provider dispatcher.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    Cycle *cycle = state;
    if (!strcmp(path, "."))
    {
        memset(output, 0, sizeof(*output));
        output->directory = 1;
        output->mode = 0555;
        return FVM_OK;
    }
    cycle->calls++;
    return dvm_provider_info(cycle->next, path, output);
}


/* EN/RU: Recursive open must stop before publishing any handle; state/path/mode/output borrowed through return.
 * Recursive open останавливается до выдачи handle; inputs borrowed до return.
 */
static FVM_Result open_cycle(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    return dvm_provider_open(((Cycle *)state)->next, path, mode, output);
}


/* EN/RU: Fixture owns stack state; consuming a factory must not free it or traverse borrowed recursion edges.
 * Fixture владеет stack state; factory не освобождает его и не обходит borrowed edges.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps operations = {info, open_cycle, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Public native cycle source. */


/* EN/RU: Validate native union candidates, failure identity and owner-first launch without a tree scan or shared node flags.
 * Проверить union candidates, failure identity и owner-first launch без tree scan/shared node flags.
 */
int main(void)
{
    Cycle a = {0};
    Cycle b = {0};
    DVM_Provider *left = dvm_provider_create_native(&operations, &a, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    DVM_Provider *right = dvm_provider_create_native(&operations, &b, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    launch_check(left && right, "cycle factories");
    a.next = left;
    FVM_FileInfo metadata;
    launch_check(dvm_provider_info(left, "dev/keyboard", &metadata) == DVM_VFS_CYCLE && a.calls == 1 && !walk_depth(),
        "self-cycle reached callback twice or leaked history");
    a.next = right;
    b.next = left;
    launch_check(dvm_provider_info(left, "dev/keyboard", &metadata) == DVM_VFS_CYCLE && a.calls == 2 && b.calls == 1 && !walk_depth(),
        "two-provider cycle was not detected");
    DVM_VfsHandle opened = {0};
    launch_check(dvm_provider_open(left, "dev/keyboard", "r", &opened) == DVM_VFS_CYCLE && !opened.ops && !walk_depth(),
        "open cycle published a handle or leaked scope");
    LaunchState terminal = {.terminal = 1, .value = 'T'};
    LaunchState provider = {.value = 'P'};
    DVM_LaunchOptions options = launch_options(&terminal, &provider);
    options.policy = DVM_LAUNCH_OWNER_FIRST;
    DVM_Namespace *group = dvm_launch_prepare(&options);
    launch_check(group && dvm_stack_mount(group, left, "/", DVM_NS_BEFORE, 0) == 1, "cyclic top union mount");
    DVM_ResourcePlan plan;
    launch_check(dvm_resource_plan(group, "/dev/keyboard", 0, &plan) &&
        dvm_resource_info(&plan, &metadata) == FVM_OK && plan.provider == options.provider && !walk_depth(),
        "cyclic branch prevented valid owner-first union lookup");
    dvm_resource_close(&plan);
    launch_check(dvm_resource_plan(group, "/absent", 0, &plan) && dvm_resource_info(&plan, &metadata) == DVM_VFS_CYCLE,
        "exhausted cycle was hidden as missing");
    dvm_resource_close(&plan);
    a.next = options.terminal;
    launch_check(dvm_resource_plan(group, "/dev/keyboard", 0, &plan) &&
        dvm_resource_info(&plan, &metadata) == FVM_OK, "later lookup retained a completed cycle mark");
    dvm_resource_close(&plan);
    dvm_namespace_release(group);
    dvm_provider_release(left);
    dvm_provider_release(right);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    launch_check(terminal.destroyed == 1 && provider.destroyed == 1 && !walk_depth(), "provider/cycle owner drain");
    puts("PASS real provider cycles, branch continuation and owner-first module environment");
    return 0;
}
