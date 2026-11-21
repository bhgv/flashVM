# Copyright (c) 2026 bhgv
# EN: Verify schema-selected ref snapshots and write-back across two DisVMs.
# RU: Проверить снимки и обратную запись ref по схеме между двумя DisVM.
implement Client;
include "sys.m";
include "compiled-ref.m";

Client: module { init: fn(); };


# EN: Check all result widths, aliasing, nil, exceptions, and conflicting edits.
# RU: Проверить размеры результата, совпадение ссылок, nil, исключение и конфликт.
init()
{
    sys := load Sys Sys->PATH;
    target := load Compiledref "/peer/mounted/compiled-ref.dis";
    if (sys->open("expect-legacy-compiled-ref-refusal", Sys->OREAD) != nil) {
        if (target != nil) {
            raise "schema-selected ref accepted an untyped import";
        }
        sys->print("PASS compiled ref requires local import types\n");
        return;
    }
    if (target == nil) {
        raise "could not load compiled ref module";
    }
    min := -big 9223372036854775807 - big 1;
    first := ref Cell(7, min);
    if (target->bump(first) != 1 || first.narrow != 8 || first.wide != min) {
        raise "single ref int result failed";
    }
    target->revise((first, first));
    (result, wide) := target->inspect((first, first));
    if (result != -2 || wide != min || first.narrow != 11) {
        raise "tuple ref lost alias identity or shared state";
    }
    second := ref Cell(11, min);
    target->revise((first, second));
    (result, wide) = target->inspect((first, second));
    if (result != 3 || wide != min || first.narrow != 12 || second.narrow != 13) {
        raise "tuple ref collapsed distinct objects";
    }
    first.narrow = -11;
    {
        target->revise((first, second));
        raise "expected owner exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (first.narrow != -11 || second.narrow != 13) {
        raise "failed void call applied copy-back";
    }
    {
        target->revise((first, nil));
        raise "expected nil ref rejection";
    } exception {
        "invalid remote ref argument" =>
            ;
    }
    first.narrow = -10;
    done := chan of int;
    spawn edit_second(sys, second, done);
    {
        target->revise((first, second));
        raise "expected a local ref conflict";
    } exception {
        "remote ref changed during call" =>
            ;
    }
    if (<-done != 1 || first.narrow != -10 || second.narrow != 901) {
        raise "conflicting void call applied partial changes";
    }
    if (target->bump(second) != 6 || second.narrow != 902 || second.wide != min) {
        raise "ref path did not recover or sent a nil argument";
    }
    sys->print("PASS mounted compiled ref argument layouts\n");
}


# EN: Change the second object after the provider takes the original snapshot.
# RU: Изменить второй объект после получения исходного снимка владельцем.
edit_second(sys: Sys, value: ref Cell, done: chan of int)
{
    wait_marker(sys, "refpair-entered");
    value.narrow = 901;
    set_marker(sys, "refpair-edited");
    done <-= 1;
}

include "refpair-sync.b";
