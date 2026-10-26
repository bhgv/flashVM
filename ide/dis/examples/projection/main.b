# Real Dis channel acceptance for inspector projections, including independent owners.
implement Projectioncheck;
include "sys.m";
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
                                          "Привет 😀", permit, 19, reply);
    result := <-reply;
    if(result.id != 42 || result.site != 19){
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
    if(s == nil || p == nil){
        raise "fail: projection modules unavailable";
    }
    a := p->start(3);
    b := p->start(4);
    r := call(a, Ideprojection->Describe, "", 0, 0, 3);
    if(r.status != Ideprojection->Ok || r.text != "number:int;text:string;add(int):int"){
        raise "fail: object expansion";
    }
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if(r.number != 7 || r.revision != 0){
        raise "fail: initial scalar";
    }
    r = call(a, Ideprojection->Invoke, "add", 5, 0, 3);
    if(r.status != Ideprojection->Denied || r.revision != 0){
        raise "fail: implicit method execution";
    }
    r = call(a, Ideprojection->Invoke, "add", 5, 1, 3);
    if(r.status != Ideprojection->Ok || r.number != 12 || r.revision != 1){
        raise "fail: owner method invocation";
    }
    r = call(a, Ideprojection->Write, "number", 99, 0, 2);
    if(r.status != Ideprojection->Stale || r.revision != 1){
        raise "fail: stale write admitted";
    }
    r = call(a, Ideprojection->Write, "text", 0, 0, 3);
    r = call(a, Ideprojection->Read, "text", 0, 0, 3);
    if(r.status != Ideprojection->Ok || r.text != "Привет 😀"){
        raise "fail: Unicode projection";
    }
    r = call(a, Ideprojection->Pause, "", 0, 0, 3);
    r = call(a, Ideprojection->Invoke, "add", 8, 1, 3);
    if(r.status != Ideprojection->Paused){
        raise "fail: method ran while paused";
    }
    r = call(a, Ideprojection->Write, "number", 123, 0, 3);
    if(r.status != Ideprojection->Paused){
        raise "fail: write ran while paused";
    }
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if(r.number != 12 || r.status != Ideprojection->Ok){
        raise "fail: paused inspection";
    }
    r = call(b, Ideprojection->Read, "number", 0, 0, 4);
    if(r.number != 7){
        raise "fail: independent owner corrupted";
    }
    r = call(a, Ideprojection->Resume, "", 0, 0, 3);
    r = call(a, Ideprojection->Write, "number", 21, 0, 3);
    r = call(a, Ideprojection->Read, "number", 0, 0, 3);
    if(r.number != 21 || r.revision != 3){
        raise "fail: resumed write";
    }
    r = call(a, Ideprojection->Release, "", 0, 0, 3);
    r = call(b, Ideprojection->Release, "", 0, 0, 4);
    checkmodel();
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
    if(m == nil){
        raise "fail: inspector model unavailable";
    }
    model := m->start();
    selected := modelcall(model, Ideinspect->Select, 3, 0, nil);
    pending := modelcall(model, Ideinspect->Begin, 0, 0, nil);
    old := pending.value.ticket;
    pending = modelcall(model, Ideinspect->Begin, 0, 0, nil);
    current := pending.value.ticket;
    late := ref Ideprojection->Reply(old, 19, Ideprojection->Ok, 3, 0, 999, "");
    rejected := modelcall(model, Ideinspect->Apply, 0, old, late);
    if(rejected.status != Ideinspect->Stale){
        raise "fail: old refresh overwrote model";
    }
    mismatched := ref Ideprojection->Reply(current, 19, Ideprojection->Ok, 4, 0, 999, "");
    rejected = modelcall(model, Ideinspect->Apply, 0, current, mismatched);
    if(rejected.status != Ideinspect->Stale){
        raise "fail: wrong owner incarnation admitted";
    }
    mismatched = ref Ideprojection->Reply(old, 19, Ideprojection->Ok, 3, 0, 999, "");
    rejected = modelcall(model, Ideinspect->Apply, 0, current, mismatched);
    if(rejected.status != Ideinspect->Stale){
        raise "fail: wrong response correlation admitted";
    }
    target := p->start(3);
    reply := chan of Ideprojection->Reply;
    target <-= ref Ideprojection->Request(current, 3, Ideprojection->Read, "number", 0,
                                        "", 0, 19, reply);
    actual := <-reply;
    accepted := modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if(accepted.status != Ideinspect->Ok || accepted.value.number != 7 || accepted.value.pending){
        raise "fail: real owner result not admitted";
    }
    # A renderer can mutate its copied row without changing model-owned state.
    accepted.value.number = 1000;
    snapshot := modelcall(model, Ideinspect->Snapshot, 0, 0, nil);
    if(snapshot.value.number != 7){
        raise "fail: mutable renderer alias";
    }
    duplicate := modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if(duplicate.status != Ideinspect->Stale){
        raise "fail: duplicate result admitted";
    }
    selected = modelcall(model, Ideinspect->Select, 4, 0, nil);
    duplicate = modelcall(model, Ideinspect->Apply, 0, current, ref actual);
    if(duplicate.status != Ideinspect->Stale){
        raise "fail: previous target reply admitted";
    }
    released := call(target, Ideprojection->Release, "", 0, 0, 3);
    stopped := modelcall(model, Ideinspect->Shutdown, 0, 0, nil);
}
