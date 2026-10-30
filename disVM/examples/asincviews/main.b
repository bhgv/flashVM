implement CaptureDemo;
include "sys.m";
include "asincviews.m";
CaptureDemo: module { init: fn(); }; # Portable protocol example, not AS syntax execution.
service: Asincviews; # Read-only loaded module handle.
requests: chan of Asincviews->Request; # Sole publication owner.
revision: int; # Updated from every consumed acknowledgement.


# Send a typed command and require admission before using the returned snapshot.
# Each call owns its reply channel; strings and flat arrays carry no native pointers.
command(operation, task, binding: int, value: string): Asincviews->Reply
{
    replies := chan of Asincviews->Reply;
    requests <-= Asincviews->Request(1, revision, operation, task, binding, value, replies);
    result := <-replies;
    if (result.status != service->Ok) {
        raise "fail: capture example request refused";
    }
    revision = result.revision;
    return result;
}


# Demonstrate capture, dirty publication, remote observation and task-local writes
# during a frame. Boundaries are explicit messages, not actual timeline hooks.
init()
{
    sys := load Sys Sys->PATH;
    service = load Asincviews Asincviews->PATH;
    if (service == nil) {
        raise "fail: capture example module missing";
    }
    requests = service->start();
    revision = 0;
    command(service->Snapshot, 0, 0, "");
    command(service->Spawn, 1, 0, "");
    command(service->Subscribe, 0, 0, "on");

    # The task replaces its own immutable cell; the owner still sees the old value.
    local := command(service->Write, 1, 0, "from task");
    if (local.values[0] != "") {
        raise "fail: task value leaked before publication";
    }
    before := command(service->BeforeFrame, 0, 0, "");
    if (before.values[0] != "from task" || before.dirty[0] != 0) {
        raise "fail: before-frame publication or no-echo rule";
    }

    # While the frame computes another binding, the task changes its private value.
    command(service->Write, 0, 1, "from frame");
    command(service->Write, 1, 0, "during frame");
    after := command(service->AfterFrame, 0, 0, "");
    if (after.values[service->Bindings] != "during frame" ||
        after.dirty[service->Bindings] != 1 || after.values[service->Bindings + 1] != "from frame") {
        raise "fail: after-frame publication overwrote private task write";
    }

    # The queue contains a publication event, not a callback executing in the writer.
    events := command(service->Drain, 0, 0, "").events;
    if (len events != 1 || events[0].source != 1 || events[0].value != "from task") {
        raise "fail: remote event source or value";
    }
    sys->print("published binding=%d source=%d version=%d value=%s\n",
               events[0].binding, events[0].source, events[0].version, events[0].value);

    # The next boundary publishes the task's later write exactly once.
    command(service->BeforeFrame, 0, 0, "");
    command(service->AfterFrame, 0, 0, "");
    events = command(service->Drain, 0, 0, "").events;
    if (len events != 1 || events[0].value != "during frame") {
        raise "fail: next task publication missing";
    }
    command(service->BeforeFrame, 0, 0, "");
    command(service->AfterFrame, 0, 0, "");
    if (len command(service->Drain, 0, 0, "").events != 0) {
        raise "fail: unchanged cells echoed";
    }

    # Retire removes this view's roots/subscriptions; Shutdown acknowledges owner exit.
    command(service->Retire, 1, 0, "");
    command(service->Shutdown, 0, 0, "");
    sys->print("PASS capture example: dirty-only phase exchange and remote publication events\n");
}
