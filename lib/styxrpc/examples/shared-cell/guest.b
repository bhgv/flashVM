# Copyright (c) 2026 bhgv
# EN: Hold a Limbo alias to the C host's cell and verify the same value through both leases.
# RU: Удержать ссылку Limbo на ячейку C и проверить общее значение через обе аренды.
implement Guest;
include "sys.m";
include "styxnamed.m";

Guest: module { init: fn(); };
sys: Sys; # Supplies namespace binding and output.
named: Styxnamed; # Opens named channels and attaches cell descriptors.
Port, Cell: import named;


# EN: Read the exact Boolean frame through the borrowed lease and compare its value.
# RU: Прочитать точный кадр Boolean через заимствованную аренду и сверить значение.
check(cell: ref Cell, expected: int)
{
    frame := cell.get(); # Fetch a fresh value through the Limbo lease.
    if (len frame != 2 || int frame[0] != Styxnamed->Boolean || int frame[1] != expected) {
        raise "unexpected shared cell Boolean";
    }
}


# EN: Read a counted single-byte text notification from the borrowed channel.
# RU: Принять однонобайтовое текстовое уведомление из заимствованного канала.
await(port: ref Port, expected: int)
{
    note := port.recv();
    if (len note != 2 || int note[0] != Styxnamed->Text || int note[1] != expected) {
        raise "unexpected notification";
    }
}


# EN: Confirm d was consumed, then stay attached while the host reads e.
# The host independently checks e, even if shutdown interrupts the final wait.
# RU: Подтвердить получение d и удерживать подключение, пока хост читает e.
# Хост проверяет e даже при разрыве соединения во время последнего ожидания.
finish(port: ref Port)
{
    {
        port.send(array[] of {byte Styxnamed->Text, byte 'e'});
        extra := port.recv();
    } exception {
        "invalid named channel request" or "named channel request failed:*"
        or "named channel reply failed:*" or "seek named channel reply:*" =>
            return;
    }
    raise "unexpected extra message after host completion";
}


# EN: Use the raw mount's named service and verify C sees the same cell.
# RU: Использовать именную службу raw-монтирования и проверить общую с C ячейку.
init()
{
    sys = load Sys Sys->PATH;
    named = load Styxnamed Styxnamed->PATH;
    if (named == nil) {
        raise "cannot load styxnamed.dis";
    }
    if (sys->bind("/device", "/", Sys->MBEFORE) < 0) {
        raise sys->sprint("cannot expose the mounted .channels service: %r");
    }
    named->init();
    port := named->open("/settings");
    cell := named->adopt(port.recv()); # Attach before the next request drops the transfer pin.
    check(cell, 1);
    cell.set(array[] of {byte Styxnamed->Boolean, byte 0});
    check(cell, 0);
    port.send(array[] of {byte Styxnamed->Text, byte 'a'});
    await(port, 'r'); # The host already verified a cancelled write and committed a new one.
    check(cell, 1);
    cell.close();
    port.send(array[] of {byte Styxnamed->Text, byte 'c'});
    await(port, 'd'); # The host has checked token revocation with both leases closed.
    finish(port);
    port.close();
    sys->print("PASS Limbo observed both values through the same lease\n");
}
