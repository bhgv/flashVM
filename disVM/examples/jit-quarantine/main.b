# Copyright (c) 2026 bhgv
# EN: A caught backend defect quarantines one function while independent functions stay native.
# RU: Пойманный backend defect карантинит одну функцию; независимые функции остаются native.
implement QuarantineDemo;
include "sys.m";
include "processctl.m";
QuarantineDemo: module { init: fn(); };
sys: Sys; # Shared immutable builtin instance.
control: Processctl; # Parent observes exact child retirement.


# EN/RU: Return input plus7; test probe injects one backend exception before this addition.
# Вернуть input плюс7; test probe вводит backend exception до этого сложения.
bad(input: int): int
{
    value := input;
    for (round := 0; round < 7; round++) {
        value++;
    }
    return value;
}


# EN/RU: Independent function must retain JIT; return input plus11.
# Независимая функция сохраняет JIT; вернуть input плюс11.
good(input: int): int
{
    value := input;
    for (round := 0; round < 11; round++) {
        value++;
    }
    return value;
}


# EN/RU: Execute the same quarantined function from an independent native owner.
# Выполнить ту же quarantined функцию в независимом native owner.
worker(done: chan of (int, int))
{
    done <-= (bad(5), control->pid());
}


# EN: Run bad twice, catch only the injected defect, verify subsequent interpretation.
# Without the test probe this example executes normally with no quarantine.
# RU: Вызвать bad дважды, поймать только injected defect, проверить interpretation.
# Без test probe пример выполняется обычно без карантина.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    caught := 0;
    {
        if (bad(1) != 8) {
            raise "wrong initial result";
        }
    } exception {
        "*" => caught = 1;
    }
    if (bad(2) != 9 || good(3) != 14) {
        raise "quarantine changed program result";
    }
    done := chan of (int, int);
    for (id := 0; id < 8; id++) {
        spawn worker(done);
    }
    for (id = 0; id < 8; id++) {
        (value, pid) := <-done;
        if (value != 12) {
            raise "cross-owner quarantine changed result";
        }
        status := control->killwait(pid);
        if (status != Processctl->Retired && status != Processctl->Denied) {
            raise "cross-owner quarantine retirement failed";
        }
    }
    sys->print("QUARANTINEDEMO caught=%d results=9,14\n", caught);
}
