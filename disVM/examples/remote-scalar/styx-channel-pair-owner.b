# Copyright (c) 2026 bhgv
# EN: Match both ends of an unbuffered proxy between provider residents.
# RU: Сопоставить оба конца небуферизованного прокси между процессами провайдера.
implement Channelpair;
include "channel-pair.m";
include "sys.m";


# EN: Send an int to the selected typed ALT, then report completion locally.
# RU: Передать int в выбранную типизированную ветку alt и сообщить о завершении.
send_integer(wire, done: chan of int)
{
    wire <-= -2147483647 - 1;
    done <-= 1;
}


# EN: Receive the scalar from a proxy ALT that also contains a string guard.
# RU: Принять скаляр из alt с прокси, где также присутствует строковая ветка.
integer(wire: chan of int, unused: chan of string, probe: int): int
{
    done := chan of int;
    spawn send_integer(wire, done);
    if (probe) {
        sys := load Sys Sys->PATH;
        for (i := 0; i < 100; i++) {
            alt {
                value := <-wire =>
                    return value == -2147483647 - 1 && <-done == 1;
                text := <-unused =>
                    return 0;
                * =>
                    sys->sleep(1);
            }
        }
        return 0;
    }
    alt {
        value := <-wire =>
            return value == -2147483647 - 1 && <-done == 1;
        text := <-unused =>
            return 0;
    }
}


# EN: Select a typed proxy send; leave the unmatched int send untouched.
# RU: Выбрать типизированную отправку через прокси и оставить вторую ветку без передачи.
send_managed(wire: chan of Payload, unused, done: chan of int)
{
    bytes := array[1500] of byte;
    bytes[0] = byte 3;
    bytes[1499] = byte 241;
    item := ref Item("shared");
    alt {
        wire <-= ("Юнибас", bytes, item, item) =>
            done <-= 1;
        unused <-= 17 =>
            done <-= 0;
    }
}


# EN: Receive the full typed frame and verify that its two references still alias.
# RU: Принять весь типизированный кадр и проверить, что две ссылки совпадают.
managed(wire: chan of Payload, unused: chan of int): int
{
    done := chan of int;
    spawn send_managed(wire, unused, done);
    (name, bytes, first, second) := <-wire;
    return <-done == 1 && name == "Юнибас" && len bytes == 1500 &&
        bytes[0] == byte 3 && bytes[1499] == byte 241 &&
        first != nil && first == second && first.label == "shared";
}
