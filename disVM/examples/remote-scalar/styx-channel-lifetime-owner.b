# Copyright (c) 2026 bhgv
# EN: Produce resident channels and combine a local wait with a remote choice.
# RU: Создавать резидентные каналы и сочетать локальное ожидание с удалённым выбором.
implement Chainchannellifetime;
include "sys.m";
include "chain-channel-lifetime.m";


# EN: Use one isolated local channel and the supplied remote channel in a blocking ALT.
# Return the winning branch and value. entered may be nil for calls without synchronization.
# Modes 0–6 exercise local readiness, delays, cancellation, races and pairing on either VM.
# RU: Использовать выделенный локальный и переданный удалённый канал в блокирующем ALT.
# Вернуть номер выбранной ветки и значение. entered может быть nil, если синхронизация не нужна.
# Режимы 0–6 проверяют готовность, задержку, отмену, гонки и встречный ALT на обеих VM.
mixed(mode: int, remote, entered: chan of int): (int, int)
{
    if (entered != nil) {
        entered <-= 19;
    }
    if (mode == 6) {
        return remote_pair(remote);
    }
    local := chan of int;
    done := chan[1] of int;
    if (mode == 0 || mode == 5) {
        local = chan[1] of int;
        local <-= 71;
    }
    if (mode == 1) {
        spawn delayed_local(local);
    }
    if (mode == 3) {
        spawn mixed_sender(local, remote, done);
    }
    alt {
        value := <-local =>
            if (mode == 3 && <-done != 1) {
                raise "mixed sender selected its idle remote branch";
            }
            return (1, value);
        value := <-remote =>
            if (mode == 5) {
                alt {
                    untouched := <-local =>
                        if (untouched != 71) {
                            raise "mixed ALT changed the unselected local payload";
                        }
                    * => raise "mixed ALT consumed both local and remote branches";
                }
            }
            return (2, value);
    }
}


# EN: Wait out several native probe opportunities, then send to the local branch.
# RU: Дать выполниться нескольким нативным проверкам, затем послать в локальную ветку.
delayed_local(local: chan of int)
{
    sys := load Sys Sys->PATH;
    sys->sleep(250);
    local <-= 72;
}


# EN: Offer on the same unbuffered local channel as mixed() and an idle remote receive.
# Signal which guard committed via done without leaving a sender behind.
# RU: Предложить отправку в тот же локальный канал без буфера и приём из пустого удалённого.
# Сообщить через done, какая ветка выполнена, и выйти из потока.
mixed_sender(local, remote, done: chan of int)
{
    alt {
        local <-= 73 =>
            done <-= 1;
        value := <-remote =>
            done <-= 0;
    }
}


# EN: Pair provider children on the caller's unbuffered channel while the caller
# waits for this result. No local peer participates in either ALT.
# RU: Связать дочерние потоки провайдера через небуферизованный канал клиента.
# Клиент ждёт итог функции, у локальных веток обоих ALT нет партнёров.
remote_pair(remote: chan of int): (int, int)
{
    sent := chan[1] of int;
    received := chan[1] of int;
    spawn mixed_remote_sender(remote, sent);
    spawn mixed_remote_receiver(remote, received);
    if (<-sent != 1) {
        raise "mixed sender chose an unready local guard";
    }
    return (2, <-received);
}


# EN: Send 76 to the caller's remote channel and report the winning guard.
# RU: Отправить 76 в канал клиента и сообщить, какая ветка выполнена.
mixed_remote_sender(remote, sent: chan of int)
{
    idle := chan of int;
    alt {
        remote <-= 76 =>
            sent <-= 1;
        value := <-idle =>
            sent <-= 0;
    }
}


# EN: Receive the paired value. This resident has its own independent idle guard.
# RU: Принять парное значение. У этого потока своя независимая локальная ветка.
mixed_remote_receiver(remote, received: chan of int)
{
    idle := chan of int;
    alt {
        value := <-remote =>
            received <-= value;
        value := <-idle =>
            received <-= -1;
    }
}


# EN: Publish one newly allocated buffered channel twice, with value already queued.
# RU: Вернуть две ссылки на один новый буферизованный канал с начальным значением.
buffered(value: int): (chan of int, chan of int)
{
    port := chan[1] of int;
    port <-= value;
    return (port, port);
}


# EN: Return two new unbuffered ends. The resident will exchange only after return.
# RU: Вернуть два новых канала без буфера. Поток проведёт обмен после возврата.
worker(start: int): (chan of int, chan of int)
{
    input := chan of int;
    output := chan of int;
    spawn serve(start, input, output);
    return (input, output);
}


# EN: Receive one value after publication, add start and complete the reply.
# RU: Принять одно значение после публикации, прибавить start и передать ответ.
serve(start: int, input, output: chan of int)
{
    value := <-input;
    output <-= value+start;
}
