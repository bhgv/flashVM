# Real Dis channel acceptance for inspector projections, including independent owners.
implement Projectioncheck;
include "sys.m";
include "idevalue.m";
include "ideprojection.m";
include "ideinspect.m";


Projectioncheck: module
{
    init: fn(); # Native runner entry, no arguments.
};
p: Ideprojection; # Loaded pure channel owner module; no native target object access.


# Send one typed inspector command and await exactly its reply. endpoint is borrowed.
# Returns the copied owner result; this test intentionally consumes every rendezvous.
call(endpoint: chan of ref Ideprojection->Request, operation: int, member: string,
     number, permit, epoch: int): Ideprojection->Reply
{
    reply := chan of Ideprojection->Reply;
    endpoint <-= ref Ideprojection->Request(42, epoch, operation, member, number,
                                          "Привет 😀", permit, 19, 0, nil, reply);
    result := <-reply;
    if (result.id != 42 || result.site != 19) {
        raise "fail: projection correlation or occurrence lost";
    }
    return result;
}


# Exercise reads, explicit method calls, stale incarnation, pause and release on real channels.
# No network or FlashVM heap is simulated by this example: these are separate Dis processes.
init()
{
    s := load Sys Sys->PATH;
    p = load Ideprojection Ideprojection->PATH;
    if (s == nil || p == nil) {
        raise "fail: projection modules unavailable";
    }
    a := p->start(3);
    b := p->start(4);
    r := call(a, Ideprojection->Describe, "", 0, 0, 3);
    if (r.status != Ideprojection->Ok || r.text != "number:int;text:string;add(int):int") {
        raise "fail: object expansion";
    }
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if (r.number != 7 || r.revision != 0) {
        raise "fail: initial scalar";
    }
    r = call(a, Ideprojection->Invoke, "add", 5, 0, 3);
    if (r.status != Ideprojection->Denied || r.revision != 0) {
        raise "fail: implicit method execution";
    }
    r = call(a, Ideprojection->Invoke, "add", 5, 1, 3);
    if (r.status != Ideprojection->Ok || r.number != 12 || r.revision != 1) {
        raise "fail: owner method invocation";
    }
    r = call(a, Ideprojection->Write, "number", 99, 0, 2);
    if (r.status != Ideprojection->Stale || r.revision != 1) {
        raise "fail: stale write admitted";
    }
    r = call(a, Ideprojection->Write, "text", 0, 0, 3);
    r = call(a, Ideprojection->Read, "text", 0, 0, 3);
    if (r.status != Ideprojection->Ok || r.text != "Привет 😀") {
        raise "fail: Unicode projection";
    }
    r = call(a, Ideprojection->Pause, "", 0, 0, 3);
    r = call(a, Ideprojection->Invoke, "add", 8, 1, 3);
    if (r.status != Ideprojection->Paused) {
        raise "fail: method ran while paused";
    }
    r = call(a, Ideprojection->Write, "number", 123, 0, 3);
    if (r.status != Ideprojection->Paused) {
        raise "fail: write ran while paused";
    }
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if (r.number != 12 || r.status != Ideprojection->Ok) {
        raise "fail: paused inspection";
    }
    r = call(b, Ideprojection->Read, "number", 0, 0, 4);
    if (r.number != 7) {
        raise "fail: independent owner corrupted";
    }
    r = call(a, Ideprojection->Resume, "", 0, 0, 3);
    r = call(a, Ideprojection->Write, "number", 21, 0, 3);
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if (r.number != 21 || r.revision != 3) {
        raise "fail: resumed write";
    }
    r = call(a, Ideprojection->Release, "", 0, 0, 3);
    r = call(b, Ideprojection->Release, "", 0, 0, 4);
    checkmodel();
    checkvalues();
    s->print("PASS projection inspector: typed owners, reads, writes, invocation, pause, stale epoch, release\n");
}


# Exercise model selection/refresh admission on a real owner channel.
# result is borrowed until the reply; all records returned by the model are independent copies.
modelcall(model: chan of ref Ideinspect->Request, operation, epoch, ticket: int,
          result: ref Ideprojection->Reply): Ideinspect->Reply
{
    reply := chan of Ideinspect->Reply;
    model <-= ref Ideinspect->Request(operation, 0, epoch, "number", ticket, result, reply);
    return <-reply;
}


# Verify stale refreshes and changed selections cannot overwrite the visible watch row.
# An actual target owner produces the admitted reply; no source evaluator is involved.
checkmodel()
{
    m := load Ideinspect Ideinspect->PATH;
    if (m == nil) {
        raise "fail: inspector model unavailable";
    }
    model := m->start();
    selected := modelcall(model, Ideinspect->Select, 3, 0, nil);
    pending := modelcall(model, Ideinspect->Begin, 0, 0, nil);
    old := pending.value.ticket;
    pending = modelcall(model, Ideinspect->Begin, 0, 0, nil);
    current := pending.value.ticket;
    late := ref Ideprojection->Reply(old, 19, Ideprojection->Ok, 3, 0, 999, "", nil);
    rejected := modelcall(model, Ideinspect->Apply, 0, old, late);
    if (rejected.status != Ideinspect->Stale) {
        raise "fail: old refresh overwrote model";
    }
    mismatched := ref Ideprojection->Reply(current, 19, Ideprojection->Ok, 4, 0, 999, "", nil);
    rejected = modelcall(model, Ideinspect->Apply, 0, current, mismatched);
    if (rejected.status != Ideinspect->Stale) {
        raise "fail: wrong owner incarnation admitted";
    }
    mismatched = ref Ideprojection->Reply(old, 19, Ideprojection->Ok, 3, 0, 999, "", nil);
    rejected = modelcall(model, Ideinspect->Apply, 0, current, mismatched);
    if (rejected.status != Ideinspect->Stale) {
        raise "fail: wrong response correlation admitted";
    }
    target := p->start(3);
    reply := chan of Ideprojection->Reply;
    target <-= ref Ideprojection->Request(current, 3, Ideprojection->Read, "number", 0,
                                        "", 0, 19, 0, nil, reply);
    actual := <-reply;
    accepted := modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if (accepted.status != Ideinspect->Ok || accepted.value.number != 7 || accepted.value.pending) {
        raise "fail: real owner result not admitted";
    }
    # A renderer can mutate its copied row without changing model-owned state.
    accepted.value.number = 1000;
    snapshot := modelcall(model, Ideinspect->Snapshot, 0, 0, nil);
    if (snapshot.value.number != 7) {
        raise "fail: mutable renderer alias";
    }
    duplicate := modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if (duplicate.status != Ideinspect->Stale) {
        raise "fail: duplicate result admitted";
    }
    selected = modelcall(model, Ideinspect->Select, 4, 0, nil);
    duplicate = modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if (duplicate.status != Ideinspect->Stale) {
        raise "fail: previous target reply admitted";
    }
    released := call(target, Ideprojection->Release, "", 0, 0, 3);
    stopped := modelcall(model, Ideinspect->Shutdown, 0, 0, nil);
}


# Send one explicit conversion/reference request to the native target owner.
# endpoint/value are borrowed until its reply; conversion returns copied data or an identity descriptor.
valuecall(endpoint: chan of ref Ideprojection->Request, operation: int, member: string,
          target: int, argument: ref Idevalue->Value): Ideprojection->Reply
{
    reply := chan of Ideprojection->Reply;
    endpoint <-= ref Ideprojection->Request(55, 9, operation, member, 5, "", 1, 23,
                                          target, argument, reply);
    result := <-reply;
    if (result.id != 55 || result.site != 23) {
        raise "fail: explicit value occurrence lost";
    }
    return result;
}


# Prove scalar copies are detached, snapshots retain nested object identity, and methods remain bound.
# A separate real Dis process owns the target; no object serialization/evaluation shortcut is used.
checkvalues()
{
    target := p->startnamed(9, "inspected-object");
    scalar := valuecall(target, Ideprojection->ReadAs, "number", Idevalue->Real, nil);
    if (scalar.status != Ideprojection->Ok || scalar.value == nil || scalar.value.number != 7.0) {
        raise "fail: owner tofloat projection";
    }
    scalar.value.number = 100.0;
    again := valuecall(target, Ideprojection->ReadAs, "number", Idevalue->Real, nil);
    if (again.value == nil || again.value.number != 7.0) {
        raise "fail: scalar copy mutated owner";
    }
    snapshot := valuecall(target, Ideprojection->ToTable, "", Idevalue->Table, nil);
    if (snapshot.status != Ideprojection->Ok || snapshot.value == nil || len snapshot.value.names != 3
       || snapshot.value.values[2].kind != Idevalue->Object || snapshot.value.values[2].epoch != 9) {
        raise "fail: table snapshot/reference boundary";
    }
    snapshot.value.values[0].integer = big 999;
    bound := valuecall(target, Ideprojection->ReadAs, "add", Idevalue->Method, nil);
    if (bound.status != Ideprojection->Ok || bound.value == nil || bound.value.member != "add") {
        raise "fail: bound method projection";
    }
    result := valuecall(target, Ideprojection->InvokeRef, "", 0, bound.value);
    if (result.status != Ideprojection->Ok || result.value == nil || result.value.integer != big 12) {
        raise "fail: bound receiver invocation or snapshot alias";
    }
    other := p->startnamed(9, "different-object");
    wrongowner := valuecall(other, Ideprojection->InvokeRef, "", 0, bound.value);
    if (wrongowner.status != Ideprojection->Stale || wrongowner.revision != 0) {
        raise "fail: method rebound to a different receiver";
    }
    # Admit a real typed value into the model and verify both input/output alias isolation.
    modelmodule := load Ideinspect Ideinspect->PATH;
    model := modelmodule->start();
    selected := modelcall(model, Ideinspect->Select, 9, 0, nil);
    pending := modelcall(model, Ideinspect->Begin, 0, 0, nil);
    requestid := pending.value.ticket;
    modelreply := chan of Ideprojection->Reply;
    target <-= ref Ideprojection->Request(requestid, 9, Ideprojection->ToTable, "", 0, "", 0, 23,
                                        Idevalue->Table, nil, modelreply);
    actual := <-modelreply;
    admitted := modelcall(model, Ideinspect->Apply, 0, requestid, ref actual);
    if (admitted.status != Ideinspect->Ok || admitted.value.value == nil
       || admitted.value.value.values[0].integer != big 12) {
        raise "fail: typed snapshot not admitted into inspector";
    }
    actual.value.values[0].integer = big 777;
    admitted.value.value.values[0].integer = big 888;
    rendered := modelcall(model, Ideinspect->Snapshot, 0, 0, nil);
    if (rendered.value.value == nil || rendered.value.value.values[0].integer != big 12) {
        raise "fail: inspector typed snapshot aliases sender/renderer";
    }
    stopped := modelcall(model, Ideinspect->Shutdown, 0, 0, nil);
    bound.value.epoch = 8;
    stale := valuecall(target, Ideprojection->InvokeRef, "", 0, bound.value);
    if (stale.status != Ideprojection->Stale || stale.revision != result.revision) {
        raise "fail: stale bound receiver executed";
    }
    # A constant crosses the channel as typed data, without exporting a variable/proxy.
    constant := ref Idevalue->Value(Idevalue->Text, big 0, 0.0, "9223372036854775807", "", 0, "", nil, nil);
    converted := valuecall(target, Ideprojection->Convert, "", Idevalue->Big, constant);
    if (converted.status != Ideprojection->Ok || converted.value == nil
       || converted.value.integer != big 9223372036854775807) {
        raise "fail: copied constant lost exact big payload";
    }
    typed := ref Idevalue->Value(Idevalue->Int, big 41, 0.0, "", "", 0, "", nil, nil);
    written := valuecall(target, Ideprojection->WriteValue, "number", 0, typed);
    if (written.status != Ideprojection->Ok) {
        raise "fail: copied typed assignment";
    }
    typed.integer = big 999;
    observed := valuecall(target, Ideprojection->ReadAs, "number", Idevalue->Int, nil);
    if (observed.value == nil || observed.value.integer != big 41) {
        raise "fail: copied assignment kept sender alias";
    }
    # Release uses this owner's actual incarnation, not the legacy helper's previous targets.
    reply := chan of Ideprojection->Reply;
    target <-= ref Ideprojection->Request(56, 9, Ideprojection->Release, "", 0, "", 0, 23,
                                        0, nil, reply);
    released := <-reply;
    other <-= ref Ideprojection->Request(57, 9, Ideprojection->Release, "", 0, "", 0, 23,
                                       0, nil, reply);
    otherreleased := <-reply;
    if (otherreleased.status != Ideprojection->Ok) {
        raise "fail: second value owner release";
    }
    if (released.status != Ideprojection->Ok) {
        raise "fail: value owner release";
    }
}
