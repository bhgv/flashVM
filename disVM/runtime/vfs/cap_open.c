/* Copyright (c) 2026 bhgv
 * [GC:] EN: Open an explicit root capability into an ordinary rooted Sys.FD.
 * RU: Открыть explicit root capability в обычный укоренённый Sys.FD.
 */
#include "cap_internal.h"
#include "view_internal.h"
#include "open_wait.h"
extern Type *TFD; /* Exact Sys.FD identity, initialized before guest startup. */


/* [GC:] EN/RU: Borrow captured root/path/mode frame, publish owned FD or nil; no fallback.
 * Snapshot path before native release, keep managed FD rooted until guarded publication.
 * Заимствовать frame, выдать owned FD/nil; path скопирован, FD rooted до publication. */
void Vfsexport_open(void *frame)
{
    F_Vfsexport_open call;
    char path[DVM_NS_PATH_LIMIT];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_open, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.root, DVM_CAP_ROOT);
    if (!state || !state->root || call.path == H || call.path == nil || !dvm_cap_text(call.path, path, sizeof(path)) ||
        !dvm_view_path(path, 0) || call.mode < 0 || call.mode > 2)
    {
        kwerrstr("invalid capability open path or mode");
        dvm_cap_publish(H, call.ret);
        return;
    }
    Sys_FD *result = H2D(Sys_FD *, heapz(TFD));
    result->fd = -1;
    ptradd(D2H(result));
    Setmark(D2H(result));
    if (waserror())
    {
        dvm_cap_abandon(result);
        nexterror();
    }
    DVM_ResourcePlan selected = {0};
    selected.provider = state->root;
    selected.selected = 1;
    selected.no_native_fallback = 1;
    strcpy(selected.relative, path);
    strcpy(selected.names.native, "/");
    result->fd = dvm_runtime_open_selected("/", call.mode == 0 ? "r" : call.mode == 1 ? "wo" : "rwo", 1, &selected);
    poperror();
    if (result->fd < 0)
    {
        dvm_cap_abandon(result);
        result = H;
    }
    dvm_cap_publish(result, call.ret);
}
