# Copyright (c) 2026 bhgv
# EN: Submit through the bound program path and wait for the provider outcome.
# RU: Запустить по привязанному пути и дождаться результата провайдера.
implement Client;
include "sys.m";
include "vfsnative.m";
include "vfsexport.m";
include "processctl.m";
include "chain-fd.m";

Client: module { init: fn(); };


# EN: Run the bound module and receive its result on the opened request.
# RU: Запустить забинженный модуль и получить итог на открытом запросе.
init()
{
    sys := load Sys Sys->PATH;
    vfs := load Vfsnative Vfsnative->PATH;
    exports := load Vfsexport Vfsexport->PATH;
    extended := sys->open("/enable-nested-checks", Sys->OREAD) != nil;
    shared := exports->intvar(17);
    if (extended) {
        vars := exports->record(("chain-var", shared) :: nil);
        vars = exports->overlay(vars, exports->intvar(9), "/chain-readonly", Vfsexport->Read, 0);
        if (vars == nil || exports->mount(vars, "/.var", 0, 0) != 0) {
            raise "could not publish chain variables";
        }
    }
    parked := exports->intvar(0);
    grant := exports->record(("chain-stage", exports->intvar(42)) :: ("chain-parked", parked) :: nil);
    inbound, outbound: ref Vfsexport->Root;
    if (extended) {
        inbound = exports->recordpipe();
        outbound = exports->recordpipe();
        if (inbound == nil || outbound == nil) {
            raise "cannot create record pipes";
        }
        sendonly := exports->overlay(inbound, nil, "/recv", 0, Vfsexport->Tree);
        recvonly := exports->overlay(outbound, nil, "/send", 0, Vfsexport->Tree);
        if (grant == nil || sendonly == nil || recvonly == nil) {
            raise "cannot select record pipe endpoints";
        }
        grant = exports->overlay(grant, sendonly, "/channel-to-a", Vfsexport->Read | Vfsexport->Write | Vfsexport->Execute, Vfsexport->Tree);
        grant = exports->overlay(grant, recvonly, "/channel-from-a", Vfsexport->Read | Vfsexport->Write | Vfsexport->Execute, Vfsexport->Tree);
    }
    if (grant == nil || exports->mount(grant, "/.vfs", 0, 0) != 0)
        raise "could not publish chain VFS";
    if (sys->bind("/peer/implementation.dis", "/alias.dis", Sys->MREPL) < 0) {
        raise "could not bind chain launch";
    }
    case vfs->run("/alias.dis") {
        1 => sys->print("PASS mounted Styx launch completion\n");
        -2 => raise "chain owner reported failure";
        -1 => raise "chain completion transport failed";
        * => raise "could not submit chain launch";
    }
    if (extended) {
        (valid, value) := exports->intget(shared);
        if (!valid || value != 24) {
            raise "nested calls and launches lost A's live variable";
        }
        sys->print("PASS nested Styx shared variables\n");
        cancel_nested(sys, vfs, exports, parked);
        check_nested_library(sys, exports, shared, parked);
        check_record_pipe(sys, exports, inbound, outbound);
    }
}


# EN: Change /fd locally, then pass the numbers of two ordinary open files.
# RU: Подменить /fd локально, затем передать номера двух обычных открытых файлов.
check_nested_library(sys: Sys, exports: Vfsexport, shared, parked: ref Vfsexport->Root)
{
    decoy := exports->record(("unrelated", exports->intvar(6)) :: nil);
    if (decoy == nil || exports->mount(decoy, "/fd", 0, 0) != 0) {
        raise "cannot replace local /fd for the chain fixture";
    }
    in := sys->open("/ordinary-input", Sys->OREAD);
    out := sys->create("/ordinary-output", Sys->OWRITE, 8r666);
    data := array[4] of byte;
    if (in == nil || out == nil || sys->readn(in, data, len data) != len data || string data != "skip") {
        raise "cannot open ordinary caller descriptors";
    }
    remote := load Chainfd "/peer/mounted/chain-fd.dis";
    if (remote == nil || exports->intset(parked, 0) != 1) {
        raise "cannot prepare nested library cancellation";
    }
    control := load Processctl Processctl->PATH;
    started := chan of int;
    done := chan of int;
    spawn pending_library(remote, control, started, done);
    pid := <-started;
    if (!entered(sys, exports, parked) || control->cancelwait(pid) != Processctl->Cancelled || <-done != 1) {
        raise "nested library cancellation failed";
    }
    if (remote->forward(in.fd, out.fd) != 73 || sys->read(in, data, 1) != 0) {
        raise "nested library or shared descriptor cursor failed";
    }
    (valid, value) := exports->intget(shared);
    if (!valid || value != 25) {
        raise "nested library lost the original .var";
    }
    sys->print("PASS nested Styx libraries and ordinary FDs\n");
}


# EN: Wait for C to enter the leaf, clunk A's request to B and reuse both hops.
# RU: Дождаться входа C, закрыть fid запроса A к B и повторить запуск через оба звена.
cancel_nested(sys: Sys, vfs: Vfsnative, exports: Vfsexport, parked: ref Vfsexport->Root)
{
    control := load Processctl Processctl->PATH;
    started := chan of int;
    done := chan of int;
    spawn pending_nested(vfs, control, started, done);
    pid := <-started;
    if (!entered(sys, exports, parked) || control->cancelwait(pid) != Processctl->Cancelled || <-done != -1 ||
        vfs->runargs("/alias.dis", array of byte "probe-nested") != 1) {
        raise "nested cancellation or follow-up failed";
    }
    sys->print("PASS nested Styx launch cancellation\n");
}


# EN: Wait until the leaf marks its entry in the original caller's VFS.
# RU: Дождаться, когда лист отметит вход в VFS исходного клиента.
entered(sys: Sys, exports: Vfsexport, parked: ref Vfsexport->Root): int
{
    for (i := 0; i < 600; i++) {
        (valid, marker) := exports->intget(parked);
        if (valid && marker == 1) {
            return 1;
        }
        sys->sleep(10);
    }
    return 0;
}


# EN: Wait on a library fid from a separate PID so the parent can cancel it.
# RU: Ждать fid библиотеки с отдельным PID, чтобы родитель мог его отменить.
pending_library(remote: Chainfd, control: Processctl, started, done: chan of int)
{
    started <-= control->pid();
    caught := 0;
    {
        remote->forward(-1, -1);
    } exception {
        * => caught = 1;
    }
    done <-= caught && remote->forward(-2, -2) == 29;
}


# EN: Report the PID of the caller waiting on the A-to-B launch fid.
# RU: Сообщить PID вызывающего, который ждёт ответ на fid A → B.
pending_nested(vfs: Vfsnative, control: Processctl, started, done: chan of int)
{
    started <-= control->pid();
    done <-= vfs->runargs("/alias.dis", array of byte "cancel-nested");
}

# EN: Use a pair of real local Limbo channels while the caller is in remote load.
# RU: Использовать настоящие каналы Limbo, пока вызывающий ждёт удалённую функцию.
check_record_pipe(sys: Sys, exports: Vfsexport, inbound, outbound: ref Vfsexport->Root)
{
    remote := load Chainfd "/peer/mounted/chain-fd.dis";
    receiver := exports->open(inbound, "recv", Sys->ORDWR);
    sender := exports->open(outbound, "send", Sys->ORDWR);
    if (remote == nil || receiver == nil || sender == nil) {
        raise "cannot open record pipe aliases";
    }
    input := chan of int;
    output := chan of int;
    done := chan of string;
    spawn record_pipe_relay(sys, receiver, sender, input, output, done);
    spawn record_pipe_check(input, output);
    if (remote->forward(-10 - receiver.fd, -10 - sender.fd) != 37) {
        raise "remote record pipe exchange failed";
    }
    problem := <-done;
    if (problem != nil) {
        raise problem;
    }
    sys->print("PASS live Limbo channels over Styx record pipes\n");
}


# EN: Move messages between the exported pipe and ordinary Limbo channels.
# RU: Передавать сообщения между файловыми концами и обычными каналами Limbo.
record_pipe_relay(sys: Sys, receiver, sender: ref Sys->FD, input, output: chan of int, done: chan of string)
{
    problem: string;
    {
        incoming := pipe_read(sys, receiver, 1024);
        pipe_check(incoming, pipe_test_record(0));
        input <-= int incoming[6];
        pipe_accept(sys, receiver);
        value := <-output;
        if (value != 73) {
            raise "invalid Limbo channel reply";
        }
        answer := pipe_test_record(1);
        if (sys->write(sender, answer, len answer) != len answer) {
            raise "cannot send Limbo channel reply";
        }
        pipe_check(pipe_read(sys, sender, 1), array[] of {byte 1});
    } exception failure {
        "*" => problem = failure;
    }
    done <-= problem;
}


# EN: Consume the incoming record in a real ALT and send the reply on a channel.
# RU: Принять входную запись в настоящем alt и послать ответ через канал.
record_pipe_check(input, output: chan of int)
{
    alt {
        incoming := <-input =>
            if (incoming != 42) {
                raise "invalid Limbo channel input";
            }
            output <-= incoming + 31;
    }
}

include "chain-record-pipe.b";
