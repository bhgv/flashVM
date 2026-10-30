implement CaptureTest;
include "sys.m";
include "asincviews.m";
CaptureTest: module { init: fn(); }; # Actual Dis scalar publication protocol regression.
views: Asincviews; # Loaded module, rooted through test lifetime.
owner: chan of Asincviews->Request; # Sole protocol owner.
revision: int; # Last accepted command lease.
checks: int; # Active oracle count, independent of NDEBUG/compiler optimization.


# Fail explicitly before using invalid state; all predicates execute in both dispatch modes.
check(condition: int, message: string)
{
    if (!condition) {
        raise "fail: asinc views " + message;
    }
    checks++;
}


# Send one request and consume its detached reply; no render frame or native pointer is used.
call(operation, task, binding: int, value: string): Asincviews->Reply
{
    replies := chan of Asincviews->Reply;
    owner <-= Asincviews->Request(1, revision, operation, task, binding, value, replies);
    result := <-replies;
    revision = result.revision;
    return result;
}


# Require admission and return copied inspection data to the caller.
accept(operation, task, binding: int, value: string): Asincviews->Reply
{
    result := call(operation, task, binding, value);
    check(result.status == views->Ok, "operation " + string operation);
    return result;
}


# Exercise opposite-direction dirty writes, atomic conflicts, no echo and snapshots.
publication()
{
    accept(views->Spawn, 1, 0, "");
    accept(views->Spawn, 2, 0, "");
    accept(views->Subscribe, 0, 0, "on");
    accept(views->Subscribe, 1, 0, "on");
    accept(views->Subscribe, 2, 0, "on");
    changed := accept(views->Write, 1, 0, "task value");
    version := changed.versions[views->Bindings];
    check(changed.values[0] == "" && changed.values[2 * views->Bindings] == "",
          "task writes remain private before frame");
    before := accept(views->BeforeFrame, 0, 0, "");
    check(before.values[0] == "task value" && before.versions[0] == version &&
          before.sources[0] == 1 && before.dirty[0] == 0 && before.dirty[views->Bindings] == 0,
          "before frame receives only task dirty version without echo");
    check(call(views->BeforeFrame, 0, 0, "").status == views->Invalid,
          "phase alternation is enforced");
    accept(views->Write, 0, 1, "frame other binding");
    accept(views->Write, 1, 0, "task during frame");
    after := accept(views->AfterFrame, 0, 0, "");
    check(after.values[views->Bindings] == "task during frame" &&
          after.dirty[views->Bindings] == 1 && after.values[views->Bindings + 1] == "frame other binding" &&
          after.dirty[views->Bindings + 1] == 0,
          "after frame preserves new task writes and distributes independent owner write");
    check(after.values[2 * views->Bindings] == "task value" && after.sources[2 * views->Bindings] == 1,
          "other task receives last published owner value and original writer identity");
    events := accept(views->Drain, 0, 0, "").events;
    check(len events == 1 && events[0].source == 1 && events[0].version == version &&
          events[0].previous == "" && events[0].value == "task value", "owner remote event payload");
    check(len accept(views->Drain, 1, 0, "").events == 0, "writer receives no own event");
    check(len accept(views->Drain, 2, 0, "").events == 1, "other task remote publication event");
    accept(views->BeforeFrame, 0, 0, "");
    accept(views->Write, 0, 0, "frame conflict");
    local := accept(views->Write, 1, 0, "task conflict");
    refused := call(views->AfterFrame, 0, 0, "");
    check(refused.status == views->Conflict && refused.revision == local.revision && refused.phase == 1 &&
          refused.values[views->Bindings] == "task conflict" && refused.values[0] == "frame conflict" &&
          refused.dirty[0] == 1 && refused.dirty[views->Bindings] == 1,
          "opposite writers conflict without partial updates");
    accept(views->Discard, 1, 0, "");
    accept(views->AfterFrame, 0, 0, "");
    accept(views->Write, 1, 0, "one");
    local = accept(views->Write, 2, 0, "two");
    refused = call(views->BeforeFrame, 0, 0, "");
    check(refused.status == views->Conflict && refused.revision == local.revision && refused.phase == 0 &&
          refused.values[0] == "frame conflict", "two task writers do not choose a silent winner");
    accept(views->Discard, 2, 0, "");
    before = accept(views->BeforeFrame, 0, 0, "");
    before.values[0] = "mutated external array";
    before.dirty[0] = 1;
    before.active[0] = -1;
    fresh := accept(views->Snapshot, 0, 0, "");
    check(fresh.values[0] == "one" && fresh.dirty[0] == 0 && fresh.active[0] == 1,
          "inspection arrays cannot mutate owner state");
    accept(views->AfterFrame, 0, 0, "");
    accept(views->Drain, 0, 0, "");
    accept(views->Drain, 1, 0, "");
    accept(views->Drain, 2, 0, "");
    accept(views->BeforeFrame, 0, 0, "");
    accept(views->AfterFrame, 0, 0, "");
    check(len accept(views->Drain, 0, 0, "").events == 0 &&
          len accept(views->Drain, 2, 0, "").events == 0, "clean boundary creates no echo events");
}


# Equal assignments retain writer version; owner changes are visible only after computation.
ownerpublication()
{
    accept(views->Subscribe, 1, 1, "on");
    before := accept(views->Snapshot, 0, 0, "");
    equal := accept(views->Write, 1, 1, before.values[views->Bindings + 1]);
    check(equal.versions[views->Bindings + 1] == before.versions[views->Bindings + 1] &&
          equal.dirty[views->Bindings + 1] == 0, "equal assignment creates no local dirty version");
    accept(views->BeforeFrame, 0, 0, "");
    changed := accept(views->Write, 0, 1, "new frame value");
    check(changed.values[views->Bindings + 1] != "new frame value",
          "owner writes stay private during computation");
    after := accept(views->AfterFrame, 0, 0, "");
    events := accept(views->Drain, 1, 0, "").events;
    check(len events == 1 && events[0].source == 0 && events[0].binding == 1 &&
          events[0].version == after.versions[1] && events[0].value == "new frame value",
          "after frame emits exact owner-origin event");
    accept(views->Subscribe, 1, 1, "off");
}


# Queue capacity is preflighted for the whole boundary, with recoverable backpressure.
capacity()
{
    accept(views->Subscribe, 1, 0, "off");
    accept(views->Subscribe, 2, 0, "off");
    for (index := 0; index < views->Capacity; index++) {
        accept(views->Write, 1, 0, "queued " + string index);
        accept(views->BeforeFrame, 0, 0, "");
        accept(views->AfterFrame, 0, 0, "");
    }
    local := accept(views->Write, 1, 0, "blocked by capacity");
    refused := call(views->BeforeFrame, 0, 0, "");
    check(refused.status == views->Full && refused.revision == local.revision && refused.phase == 0 &&
          refused.dirty[views->Bindings] == 1 && refused.values[0] != "blocked by capacity",
          "full queue refuses before publishing or consuming dirty write");
    events := accept(views->Drain, 0, 0, "").events;
    check(len events == views->Capacity && events[0].value == "queued 0" &&
          events[len events - 1].value == "queued " + string (views->Capacity - 1),
          "drain preserves FIFO publication order");
    accept(views->BeforeFrame, 0, 0, "");
    accept(views->AfterFrame, 0, 0, "");
    check(len accept(views->Drain, 0, 0, "").events == 1, "retry publishes exactly once");
}


# Late multi-recipient overflow must not update even the first task or consume frame dirty bits.
latecapacity()
{
    accept(views->Drain, 0, 0, "");
    accept(views->Subscribe, 1, 1, "on");
    accept(views->Subscribe, 2, 1, "on");
    for (index := 0; index < views->Capacity - 1; index++) {
        accept(views->Write, 1, 0, "late " + string index);
        accept(views->BeforeFrame, 0, 0, "");
        accept(views->AfterFrame, 0, 0, "");
    }
    accept(views->BeforeFrame, 0, 0, "");
    local := accept(views->Write, 0, 1, "two pending recipients");
    refused := call(views->AfterFrame, 0, 0, "");
    check(refused.status == views->Full && refused.revision == local.revision &&
          refused.phase == 1 && refused.dirty[1] == 1 &&
          refused.values[views->Bindings + 1] == local.values[views->Bindings + 1] &&
          refused.values[2 * views->Bindings + 1] == local.values[2 * views->Bindings + 1],
          "late queue overflow cannot partially publish first recipient");
    check(len accept(views->Drain, 0, 0, "").events == views->Capacity - 1,
          "refused after boundary added no events");
    accept(views->AfterFrame, 0, 0, "");
    check(len accept(views->Drain, 1, 0, "").events == 1 &&
          len accept(views->Drain, 2, 0, "").events == 1,
          "after boundary retry emits exactly one event per subscriber");
}


# Each independent Dis writer owns its reply channel and retries only exact stale leases.
writer(task, binding: int, ready: chan of int, go: chan of int, done: chan of int)
{
    replies := chan of Asincviews->Reply;
    owner <-= Asincviews->Request(1, 0, views->Snapshot, task, binding, "", replies);
    state := <-replies;
    lease := state.revision;
    ready <-= lease;
    <-go;
    retries := 0;
    for (step := 0; step < 20; step++) {
        for (;;) {
            owner <-= Asincviews->Request(1, lease, views->Write, task, binding,
                                         "writer " + string task + ":" + string step, replies);
            state = <-replies;
            lease = state.revision;
            if (state.status == views->Ok) {
                break;
            }
            if (state.status != views->Stale) {
                raise "fail: concurrent asinc writer refused unexpectedly";
            }
            retries++;
        }
    }
    done <-= retries;
}


# Real spawned processes race one lease, while independent bindings remain private until publication.
concurrentwriters()
{
    ready := chan of int;
    go := chan of int;
    done := chan of int;
    spawn writer(1, 2, ready, go, done);
    spawn writer(2, 3, ready, go, done);
    first := <-ready;
    second := <-ready;
    check(first == second, "writers start from the same immutable service revision");
    go <-= 1;
    go <-= 1;
    retries := <-done;
    retries += <-done;
    check(retries > 0, "racing stale command was actually refused and retried");
    local := accept(views->Snapshot, 0, 0, "");
    check(local.values[2] == "" && local.values[3] == "" &&
          local.values[views->Bindings + 2] == "writer 1:19" &&
          local.values[2 * views->Bindings + 3] == "writer 2:19",
          "spawned writers retain independent private views");
    published := accept(views->BeforeFrame, 0, 0, "");
    check(published.values[2] == "writer 1:19" && published.values[3] == "writer 2:19",
          "one boundary merges independently written bindings");
    after := accept(views->AfterFrame, 0, 0, "");
    check(after.values[views->Bindings + 3] == "writer 2:19" &&
          after.values[2 * views->Bindings + 2] == "writer 1:19",
          "published values propagate to other task views");
}


# Lifecycle and stale leases cannot alias task IDs or publish rejected commands.
lifetime()
{
    old := revision;
    accept(views->Subscribe, 0, 0, "off");
    replies := chan of Asincviews->Reply;
    owner <-= Asincviews->Request(7, old, views->Write, 1, 0, "stale", replies);
    stale := <-replies;
    check(stale.status == views->Stale && stale.id == 7 && stale.revision == revision,
          "stale lease retains exact current state");
    check(call(views->Write, 0, 0, "outside frame").status == views->Invalid,
          "frame owner cannot write outside computation phase");
    check(call(views->Write, views->Tasks + 1, 0, "invalid").status == views->Invalid,
          "invalid task refused");
    check(call(views->Write, 1, views->Bindings, "invalid").status == views->Invalid,
          "invalid binding refused");
    retired := accept(views->Retire, 1, 0, "");
    check(retired.active[1] == -1 && retired.versions[views->Bindings] == 0 &&
          retired.dirty[views->Bindings] == 0, "retire releases view and pending write");
    check(call(views->Spawn, 1, 0, "").status == views->Invalid,
          "retired task identity is never silently reused");
    check(call(views->Drain, 1, 0, "").status == views->Invalid,
          "retired subscriptions cannot be polled");
    accept(views->Spawn, views->Tasks, 0, "");
    accept(views->Write, views->Tasks, views->Bindings - 1, "last binding");
    last := accept(views->BeforeFrame, 0, 0, "");
    check(last.values[views->Bindings - 1] == "last binding", "last task and binding participate");
    accept(views->AfterFrame, 0, 0, "");
}


# Load ordinary bytecode and run scheduled typed-channel protocol on real DisVM.
init()
{
    sys := load Sys Sys->PATH;
    views = load Asincviews Asincviews->PATH;
    check(views != nil, "load module");
    owner = views->start();
    revision = 0;
    checks = 1;
    accept(views->Snapshot, 0, 0, "");
    publication();
    ownerpublication();
    capacity();
    latecapacity();
    concurrentwriters();
    lifetime();
    accept(views->Shutdown, 0, 0, "");
    sys->print("PASS asinc scalar views: %d checks, dirty-only boundaries, COW cells, atomic conflicts, remote events and lifetime\n", checks);
}
