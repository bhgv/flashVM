# Copyright (c) 2026 bhgv
# EN: Bounded FD IO children publish scalar completion, then retire on their own threads.
# RU: Ограниченные FD IO children публикуют scalar completion и завершаются на своих threads.
implement Proxystream;
include "sys.m";
include "processctl.m";
include "proxystream.m";
sys: Sys; # Immutable after init.
control: Processctl; # Immutable after init; killwait addresses only direct children.


# EN/RU: Initialize before any transfer children share this module.
# Инициализировать до передачи module дочерним IO-процессам.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    if (sys == nil || control == nil) {
        raise "proxy stream builtins unavailable";
    }
}


# EN/RU: Announce own PID, then complete finite alarm without needing a receiver.
# Передать собственный PID и завершить finite alarm без обязательного получателя.
alarm(started, signal: chan of int, deadline: int)
{
    started <-= control->pid();
    remaining := deadline - sys->millisec();
    if (remaining > 0) {
        sys->sleep(remaining);
    }
    signal <-= 1;
}


# EN: Borrow FD/bytes through each Sys call. Partial reads/writes continue only
# on this original stream; EOF/failure never starts another frame or replays effect.
# RU: Заимствовать FD/bytes на Sys calls. Partial IO продолжается лишь в исходном
# stream; EOF/failure не начинает другой frame и не повторяет effect.
exchange(fd: ref Sys->FD, bytes: array of byte, writing: int,
         started: chan of int, done: chan of (int, int))
{
    started <-= control->pid();
    transferred := 0;
    while (transferred < len bytes) {
        amount: int;
        if (writing) {
            amount = sys->write(fd, bytes[transferred:], len bytes - transferred);
        } else {
            amount = sys->read(fd, bytes[transferred:], len bytes - transferred);
        }
        if (amount <= 0) {
            status := Failure;
            if (amount == 0) {
                status = End;
            }
            done <-= (status, transferred);
            return;
        }
        transferred += amount;
    }
    done <-= (OK, transferred);
}


# EN: A private supervisor owns IO/alarm children through their terminal joins.
# If its caller retires, buffered completion still lets this supervisor finish;
# no orphaned native read survives the finite original deadline.
# RU: Private supervisor владеет IO/alarm до terminal join. При retirement caller
# buffered completion позволяет supervisor завершиться; orphaned native read
# не переживает конечный исходный deadline.
supervise(fd: ref Sys->FD, bytes: array of byte, writing, deadline: int,
          published: chan of int, completed: chan of (int, int))
{
    published <-= control->pid();
    started := chan[2] of int;
    signal := chan[1] of int;
    done := chan[1] of (int, int);
    spawn alarm(started, signal, deadline);
    timer := <-started;
    spawn exchange(fd, bytes, writing, started, done);
    worker := <-started;
    result := (Timeout, -1);
    alt {
        result = <-done => {}
        <-signal => {}
    }
    control->killwait(worker);
    control->killwait(timer);
    completed <-= result;
}


# EN/RU: Borrow FD/bytes through finite supervised IO; caller retirement cannot strand IO.
# Заимствовать FD/bytes на конечном supervised IO; retirement caller не оставляет IO.
transfer(fd: ref Sys->FD, bytes: array of byte, writing, deadline: int): (int, int)
{
    if (fd == nil || bytes == nil || len bytes > Limit || (writing != 0 && writing != 1)) {
        return (Failure, 0);
    }
    if (deadline <= sys->millisec()) {
        return (Timeout, 0);
    }
    published := chan[1] of int;
    completed := chan[1] of (int, int);
    spawn supervise(fd, bytes, writing, deadline, published, completed);
    supervisor := <-published;
    result := <-completed;
    control->killwait(supervisor);
    return result;
}
