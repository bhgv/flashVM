# Copyright (c) 2026 bhgv
# EN: Export one real Limbo Int32 while FlashVM cancels exact remote fid operations.
# RU: Экспортировать Int32 из Limbo и проверить отмену FlashVM на точных удалённых fid.
implement Client;
include "sys.m";
include "styxnamed.m";
include "styxnodes.m";

Client: module { init: fn(); };
sys: Sys; # Namespace, readiness markers and monotonic clock.
named: Styxnamed; # Named channel transport for the reference.
refs: Styxmodref; # Capture a reference to the exact local export.
nodes: Styxnodes; # Own the native variable and positioned requests.
Port, Message: import named;
Cell: import refs;
Node: import nodes;
Export: con "/mounted/counter.dis/.var/value"; # Canonical module variable path.
BudgetMs: con 9000; # Limit for the final movie marker.

include "named_movie_upload.b";


# EN: Serve four checked Int32 transfers at offset zero and report the final native value.
# RU: Обслужить четыре проверенных обмена Int32 со смещения ноль и вернуть итог.
serve(node: ref Node, done: chan of int)
{
    value := 17;
    for (i := 0; i < 4; ++i) {
        (item, at) := nodes->recvat(node);
        if (at != big 0 || len item.buffer != 4) {
            node.refuse(item);
            done <-= -1;
            return;
        }
        if (i == 0 || i == 2) {
            if (item.operation != "r") {
                node.refuse(item);
                done <-= -1;
                return;
            }
            item.buffer[0] = byte (value >> 24);
            item.buffer[1] = byte (value >> 16);
            item.buffer[2] = byte (value >> 8);
            item.buffer[3] = byte value;
        } else {
            next := 0;
            for (j := 0; j < 4; ++j) {
                next = next * 256 + int item.buffer[j];
            }
            if (item.operation != "w" || (i == 1 && next != 42) || (i == 3 && next != 73)) {
                node.refuse(item);
                done <-= -1;
                return;
            }
            value = next;
        }
        if (!node.reply(item, 4)) {
            done <-= -1;
            return;
        }
    }
    done <-= value;
}


# EN: Keep one publication through all six rendezvous and check final native state.
# RU: Удержать одну публикацию во всех шести встречах и проверить нативный результат.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    nodes = load Styxnodes Styxnodes->PATH;
    refs = load Styxmodref Styxmodref->PATH;
    if (named == nil || nodes == nil || refs == nil) {
        raise "cannot load Int32 reference support";
    }
    named->init();
    nodes->init();
    refs->init();
    node := nodes->publishat(Export, "rw", big 4);
    done := chan[1] of int;
    spawn serve(node, done);
    cell := refs->capturelocal(Export[1:], 1);
    start("cancel-int-ref.swf");
    for (stage := 0; stage <= 5; ++stage) {
        port := named->open("/int-ref-" + string stage);
        port.sendvalue(ref Message.Modref(cell));
        port.close();
    }
    began := sys->millisec();
    while (sys->millisec()-began < BudgetMs) {
        (ready, nil) := sys->stat("/control-done");
        if (ready == 0) {
            if (<-done != 73) {
                raise "Int32 native state or operation count differed";
            }
            cell.close();
            node.close();
            sys->print("PASS native cancel-int-ref RPC paired VMs\n");
            return;
        }
        sys->sleep(5);
    }
    raise "missing restored Int32 access marker";
}
