# Copyright (c) 2026 bhgv
# EN: Real channel close plus two admitted relay peers during GC claim retries.
# RU: Настоящее закрытие канала при двух активных соседях и повторах захвата GC.
implement ChannelClaim;
include "sys.m";
ChannelClaim: module { init: fn(); };
sys: Sys; # Published before the three permanent residents start.


# EN: Keep the real sleep frame in the host admission relay; ready/done stay rooted.
# RU: Удержать настоящий sleep-кадр в эстафете допуска; ready/done остаются корнями.
peer(ready, done: chan of int)
{
    ready <-= 1;
    if (sys->sleep(0) != 0) {
        raise "channel claim peer sleep changed";
    }
    done <-= 1;
}


# EN: Register a real receive and finish only after the owner's close message.
# channel/ready/done stay owned by this resident until the native unwind finishes.
# RU: Зарегистрировать настоящий receive и завершиться лишь после сообщения
# закрытия владельцу. channel/ready/done удерживаются до завершения unwind.
waiter(channel, ready, done: chan of int)
{
    ready <-= 1;
    {
        <-channel;
        raise "unexpected channel payload";
    } exception error {
        "channel hangup" =>
            done <-= 3;
        * =>
            raise error;
    }
}


# EN: Keep parent channels rooted while native diagnostics close the real waiter
# and reclaim independent quiet channels. Join all three permanent residents.
# RU: Удерживать каналы родителя, пока native-проверка закрывает настоящий waiter
# и освобождает отдельные спокойные каналы. Дождаться всех трёх потоков.
init()
{
    sys = load Sys Sys->PATH;
    ready := chan of int;
    done := chan of int;
    channel := chan of int;
    spawn peer(ready, done);
    spawn peer(ready, done);
    spawn waiter(channel, ready, done);
    <-ready;
    <-ready;
    <-ready;
    sys->werrstr("channel claim checkpoint");
    total := <-done;
    total += <-done;
    total += <-done;
    if (total != 5) {
        raise "channel claim residents did not finish";
    }
    sys->print("PASS resident channel claim peers=3\n");
}
