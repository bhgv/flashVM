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
    DVM_LaunchCompletion completion; /* Borrowed status target through child retirement. */
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
#endif


/* EN: Wait for at most one configured launch and leave its child running.
 * RU: Дождаться одного настроенного запуска и оставить потомка выполняться. */
void Vfsnative_acceptsharedlaunch(void *frame)
{
    (void)frame;
#ifdef DVM_TERMINAL_LAUNCH
    HostedLaunch launch = {0};
    DVM_LaunchHandoff *handoff = &launch.handoff;
    handoff->group = dvm_namespace_copy(dvm_namespace_current_group());
    if (!handoff->group)
    {
        return;
    }
    unsigned char arguments[64];
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    int accepted = released >= 0 && dvm_terminal_take_shared_launch(handoff, arguments, &launch.image, &launch.completion);
    if (released >= 0)
    {
        int cancelled = dvm_vfs_wait_end(wait);
        dvm_runtime_gc_native_enter(released);
        accepted = accepted && !cancelled;
    }
    if (launch.completion.report && (!accepted || !kprotect(start_hosted, &launch)))
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
#endif
}
