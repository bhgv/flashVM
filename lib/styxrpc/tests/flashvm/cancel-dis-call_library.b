# Copyright (c) 2026 bhgv
# EN: Consume a lent value and park a scoped Dis call on a local channel.
# RU: Принять значение из заимствованной очереди и остановить вызов на локальном канале.
implement Library;
include "sys.m";

Library: module {
    park: fn(box: ref Cell, source: chan of int): chan of int;
    parkText: fn(value: string): string;
    Cell: adt {
        value: int; # Leaf reached through the graph codec.
    };
    Box: adt {
        cell: ref Cell; # Borrowed nested reference in the checked call.
    };
    parkGraph: fn(box: ref Box): int;
    echo: fn(value: int): int;
};

Receive: con 0; # Park on a local channel receive.
Send: con 1; # Park on a local channel send.
Select: con 2; # Park on a local channel alternation.


# EN: Make an observable marker in the ordinary paired namespace.
# RU: Создать видимую отметку в обычном парном пространстве имён.
mark(sys: Sys, path: string)
{
    file := sys->create(path, Sys->OWRITE, 8r666);
    if (file == nil) {
        raise "cannot mark the scoped call";
    }
}


# EN: Park in the selected Limbo operation and mark the handler reached on unwind.
# RU: Остановиться в выбранной операции Limbo и отметить вход в обработчик исключения.
wait_local(name: string, mode: int)
{
    sys := load Sys Sys->PATH;
    blocker := chan of int;
    second := chan of int;
    {
        mark(sys, "/" + name + "-entered");
        case mode {
        Receive =>
            ignored := <-blocker;
        Send =>
            blocker <-= 41;
        Select =>
            alt {
            ignored := <-blocker =>
                ;
            second <-= 41 =>
                ;
            }
        }
        raise "local channel unexpectedly matched";
    } exception problem {
        "*" =>
            mark(sys, "/" + name + "-unwound");
            raise problem;
    }
}


# EN: Confirm receipt on the reverse bus, then wait on a fresh Limbo channel.
# RU: Подтвердить приём со встречной шины, затем ждать на новом канале Limbo.
park(box: ref Cell, source: chan of int): chan of int
{
    if (box == nil || box.value != 40 || <-source != 41) {
        raise "incorrect lent value";
    }
    box.value = 99;
    wait_local("channel", Receive);
    return nil;
}


# EN: Check a managed string and park on a local channel send.
# RU: Проверить управляемую строку и остановиться при отправке в локальный канал.
parkText(value: string): string
{
    if (value != "task") {
        raise "incorrect managed string";
    }
    wait_local("text", Send);
    return value;
}


# EN: Check the borrowed nested graph and park on a local alternation.
# RU: Проверить заимствованный вложенный граф и остановиться на локальном alt.
parkGraph(box: ref Box): int
{
    if (box == nil || box.cell == nil || box.cell.value != 40) {
        raise "incorrect graph argument";
    }
    wait_local("graph", Select);
    return box.cell.value;
}


# EN: Return a checked scalar while another resident is waiting or after it exits.
# RU: Вернуть скаляр, пока другой владелец ждёт или уже завершил работу.
echo(value: int): int
{
    return value;
}
