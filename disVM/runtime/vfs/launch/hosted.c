/* Copyright (c) 2026 bhgv
 * EN: Adopt the locally configured launch from a combined Styx session.
 * RU: Принять настроенный локально запуск из общей сессии Styx.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "kernel.h"
#include "../guest.h"
#include "runtime.h"
#include "../namespace.h"
#include "../native_wait.h"
#include "runtime_gc.h"
#include "vfsnative_decl.h"

#ifdef DVM_TERMINAL_LAUNCH
extern int dvm_terminal_take_shared_launch(DVM_LaunchHandoff *, unsigned char *, const char **, DVM_LaunchCompletion *);

typedef struct HostedLaunch
{
    DVM_LaunchHandoff handoff; /* Pending adoption transfers its namespace ownership. */
    DVM_LaunchCompletion completion; /* Owned until transferred to the child or reported. */
    const char *image; /* Borrowed host CLI image path. */
    Module *module; /* Owned loader reference, released after publication. */
} HostedLaunch;


/* EN: Load and publish the configured program in a new Dis process.
 * RU: Загрузить и опубликовать настроенную программу в новом процессе Dis. */
static int start_hosted(void *state)
{
    HostedLaunch *launch = state;
    launch->module = load((char *)launch->image);
    return launch->module && dvm_runtime_launch_child(launch->module, &launch->handoff, &launch->completion);
}
/* EN: Start one hosted child and return whether to keep accepting.
 * RU: Запустить одного потомка и сообщить, продолжать ли принимать запросы. */
static int accept_hosted(void)
{
    HostedLaunch launch = {0};
    DVM_LaunchHandoff *handoff = &launch.handoff;
    handoff->group = dvm_namespace_copy(dvm_namespace_current_group());
    if (!handoff->group)
    {
        return 0;
    }
    unsigned char arguments[64];
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    int outcome = 0, cancelled = 0;
    if (released >= 0)
    {
        outcome = dvm_terminal_take_shared_launch(handoff, arguments, &launch.image, &launch.completion);
        cancelled = dvm_vfs_wait_end(wait);
        dvm_runtime_gc_native_enter(released);
    }
    if (launch.completion.report && (outcome != 1 || cancelled || !kprotect(start_hosted, &launch)))
    {
        launch.completion.report(launch.completion.state, 1);
    }
    if (launch.module)
    {
        unload(launch.module);
    }
    dvm_provider_release(handoff->program);
    dvm_provider_release(handoff->program_parent);
    dvm_provider_release(handoff->program_vfs);
    dvm_provider_release(handoff->program_var);
    dvm_namespace_release(handoff->group);
    return outcome && !cancelled;
}
#endif


/* EN: Keep admitting independent hosted launches until the transport closes.
 * RU: Принимать независимые запуски до закрытия транспорта. */
void Vfsnative_acceptsharedlaunch(void *frame)
{
    (void)frame;
#ifdef DVM_TERMINAL_LAUNCH
    while (accept_hosted())
    {
    }
#endif
}
