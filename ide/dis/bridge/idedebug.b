implement Idedebug;
include "idedebug.m";


# Serialize session commands on a private Limbo process. epoch is immutable;
# requests is owned until acknowledged Shutdown. No target/GC roots are borrowed.
# Unsupported target operations leave every presentation value unchanged.
serve(epoch: int, requests: chan of Request)
{
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Invalid;
        stop := 0;
        if (request.id >= 0 && request.operation >= Snapshot && request.operation <= Shutdown) {
            if (request.epoch != epoch) {
                status = Stale;
            } else if (request.operation == Snapshot || request.operation == Shutdown) {
                status = Ok;
                stop = request.operation == Shutdown;
            } else {
                status = Unsupported;
            }
        }
        request.reply <-= Reply(request.id, epoch, status, 0, 0,
            "No target attached.\nSource location and breakpoint gutter await a runtime debug capability.\nNo current instruction is fabricated.",
            "No target attached.\nStack frames: unavailable.\nStep/next/continue/pause require runtime safe points; controls are not enabled.",
            "No target attached.\nLocals, modules and values: unavailable.\nNo expression or getter is evaluated by opening this panel.");
        if (stop) {
            return;
        }
    }
}


# Acquire one private channel/process for a valid epoch. Returns nil otherwise;
# startup never loads a target, executes source or claims a debugger capability.
start(epoch: int): chan of Request
{
    if (epoch <= 0) {
        return nil;
    }
    requests := chan of Request;
    spawn serve(epoch, requests);
    return requests;
}
