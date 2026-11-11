# Copyright (c) 2026 bhgv
# EN: Retire a parent while its child's cancellation reply is held by the host.
# RU: Завершить родителя, пока host удерживает ответ об отмене ожидания ребёнка.
implement ParentExit;
include "sys.m";
include "processctl.m";

ParentExit: module { init: fn(); };
sys: Sys; # Immutable interface initialized before spawn.
control: Processctl; # Read-only shared process-control module reference.


# EN: Keep mapped roots live through cancellation and acknowledge to grandparent.
# Channels are shared references; no pointer to the dying parent is retained.
# RU: Сохранить mapped-корни при отмене и ответить деду через общий канал.
# Каналы — общие ссылки; указателей на умирающего родителя здесь нет.
child(ids, gate, done: chan of int)
{
    roots := array[64] of array of int;
    for (index := 0; index < len roots; index++) {
        roots[index] = array[1] of int;
        roots[index][0] = index + 2000;
    }
    ids <-= control->pid();
    {
        alt {
            <-gate => raise "unexpected orphan publication";
        }
    } exception {
        "channel wait cancelled" => {}
    }
    for (verify := 0; verify < len roots; verify++) {
        if (roots[verify] == nil || roots[verify][0] != verify + 2000) {
            raise "orphan roots changed after parent exit";
        }
    }
    done <-= 1;
}


# EN: Request child cancellation; the host holds its reply before it can resume.
# ids/done outlive this parent. Returning from a successful cancel is a failure.
# RU: Запросить отмену ребёнка; host задерживает ответ до возобновления родителя.
# ids/done живут дольше родителя. Успешный возврат cancel — ошибка теста.
middle(ids, done: chan of int)
{
    ids <-= control->pid();
    childids := chan of int;
    gate := chan of int;
    spawn child(childids, gate, done);
    target := <-childids;
    for (;;) {
        result := control->cancelwait(target);
        if (result != Processctl->NotWaiting) {
            raise "parent resumed before its held reply was discarded";
        }
        sys->sleep(1);
    }
}


# EN: Host markers wait for the held reply and then release it after killwait.
# No arguments/result; successful orphan completion proves the late reply path.
# RU: Host-метки ждут задержанный ответ и освобождают его после killwait.
# Без аргументов/результата; завершение сироты проверяет путь позднего ответа.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    ids := chan of int;
    done := chan of int;
    spawn middle(ids, done);
    target := <-ids;
    entered := chan[101] of int;
    if (entered == nil || control->killwait(target) != Processctl->Retired ||
        control->killwait(target) != Processctl->Denied) {
        raise "parent did not retire with its cancellation reply held";
    }
    released := chan[102] of int;
    if (released == nil || <-done != 1) {
        raise "orphan failed to complete after parent retirement";
    }
    sys->print("PASS orphan cancellation after parent retirement\n");
}
