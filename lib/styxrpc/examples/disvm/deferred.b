# Copyright (c) 2026 bhgv
# EN: Cancel a mounted C callback from its real Limbo parent without stopping the other fid.
# RU: Отменить смонтированный колбэк C из процесса Limbo, сохранив соседний fid.
implement Deferred;
include "sys.m";
include "processctl.m";

Deferred: module { init: fn(); };
sys: Sys; # Hosted Sys entry points for the real mounted namespace.
ctl: Processctl; # Direct-child control authority of the guest parent.
Fields: con 8; # Exact size of the fixture's control snapshot.


# EN: Raise with the actual failing phase and current Sys error.
# RU: Остановить проверку с названием этапа и текущей ошибкой Sys.
check(ok: int, phase: string)
{
    if (!ok) {
        raise sys->sprint("%s: %r", phase);
    }
}


# EN: Read independent gate fids until a provider callback publishes its phase.
# RU: Читать независимые fid gate до публикации этапа колбэком провайдера.
await(field, count: int): array of byte
{
    status := array[Fields] of byte;
    start := sys->millisec();
    while (sys->millisec() - start < 1700) {
        fd := sys->open("/device/gate", Sys->OREAD);
        check(fd != nil && sys->read(fd, status, len status) == len status, "owner control read");
        fd = nil;
        if (int status[field] >= count) {
            check(int status[field] == count, "unexpected callback count");
            return status;
        }
        sys->sleep(2);
    }
    raise "deferred callback did not report its phase";
}


# EN: Send one command through a fresh gate fid on the provider owner.
# RU: Отправить одну команду по свежему fid gate в поток владельца провайдера.
signal(command: string)
{
    fd := sys->open("/device/gate", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array of byte command, 1) == 1, "owner control write");
}


# EN: Publish the child PID, then return a read byte or the Sys failure to its parent.
# RU: Выдать PID потомка, затем передать родителю прочитанный байт либо ошибку Sys.
reader(ready, done: chan of int)
{
    fd := sys->open("/device/data", Sys->OREAD);
    check(fd != nil, "open reader");
    ready <-= ctl->pid();
    data := array[1] of byte;
    count := sys->read(fd, data, 1);
    fd = nil;
    if (count == 1) {
        done <-= int data[0];
    } else {
        done <-= count;
    }
    idle := chan of int;
    <-idle;
}


# EN: Confirm a one-byte prefix, then park the current-cursor suffix write.
# RU: Подтвердить однобайтовый префикс, затем ждать записи со смещения курсора.
writer(ready, done: chan of int)
{
    fd := sys->open("/device/data", Sys->OWRITE);
    check(fd != nil && sys->write(fd, array of byte "AB", 2) == 1, "short write prefix");
    ready <-= ctl->pid();
    count := sys->write(fd, array of byte "B", 1);
    fd = nil;
    done <-= count;
    idle := chan of int;
    <-idle;
}


# EN: Cancel each child using its native wait. Check the sibling can finish,
# the written prefix remains and a fresh read reaches the same live C provider.
# RU: Отменить нативное ожидание каждого потомка. Проверить соседний запрос,
# сохранение префикса записи и новое чтение с того же провайдера C.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    check(ctl != nil, "load Processctl");
    rr := chan of int;
    rd := chan of int;
    wr := chan of int;
    wd := chan of int;
    spawn reader(rr, rd);
    spawn writer(wr, wd);
    readerpid := <-rr;
    writerpid := <-wr;
    await(0, 1);
    await(2, 1);
    check(ctl->cancelwait(writerpid) == Processctl->Cancelled && <-wd == -1, "cancel exact mounted write");
    status := await(3, 1);
    check(status[1] == byte 0 && status[5] == byte 'A' && status[6] == byte '_', "write prefix or sibling changed");
    check(ctl->killwait(writerpid) == Processctl->Retired, "retire writer child");
    signal("R");
    check(<-rd == int byte 'R' && ctl->killwait(readerpid) == Processctl->Retired, "unaffected reader");
    signal("P");
    spawn reader(rr, rd);
    readerpid = <-rr;
    await(0, 2);
    check(ctl->cancelwait(readerpid) == Processctl->Cancelled && <-rd == -1, "cancel exact mounted read");
    await(1, 1);
    check(ctl->killwait(readerpid) == Processctl->Retired, "retire reader child");
    signal("D");
    signal("R");
    fd := sys->open("/device/data", Sys->OREAD);
    data := array[1] of byte;
    check(fd != nil && sys->read(fd, data, 1) == 1 && data[0] == byte 'R', "reopen after cancellation");
    created := sys->create("/device/new", Sys->OWRITE | Sys->OEXCL, 8r666);
    check(created != nil && sys->write(created, array of byte "N", 1) == 1, "write new fid on worker");
    await(7, int byte 'N');
    sys->print("PASS Limbo cancelled mounted C provider reads and writes\n");
}
