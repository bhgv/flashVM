# Copyright (c) 2026 bhgv
# EN: Concurrent real/string conversions and formatting with exact binary64 oracles.
# RU: Параллельные real/string-преобразования и форматирование с точным binary64-эталоном.
implement ResidentDecimal;
include "sys.m";
include "math.m";
include "processctl.m";

ResidentDecimal: module { init: fn(); };
sys: Sys; # Immutable builtin interfaces published before spawn.
math: Math; # Bit-copy oracle, not a second decimal conversion algorithm.
control: Processctl; # Exact completion and retirement of each producer.
texts: array of string; # Read-only process-lifetime decimal inputs.
expected: array of big; # Read-only independently specified IEEE binary64 bits.
Workers: con 8; # Concurrent OS residents sharing only immutable sample data.
Rounds: con 128; # Each resident tests every sample repeatedly.


# EN: Check value against this case's immutable bits; source labels the operation.
# RU: Проверить value по неизменным битам случая; source обозначает операцию.
check(value: real, at: int, source: string)
{
    if (math->realbits64(value) != expected[at]) {
        raise "parallel decimal mismatch: " + source;
    }
}


# EN: Wait for a common start, parse/format with real VM handlers and return PID.
# id selects sample order; channels/temporary strings remain owned until exit.
# RU: Ждать общего старта, выполнять настоящие VM-преобразования и вернуть PID.
# id выбирает порядок; каналы/временные строки принадлежат потоку до выхода.
worker(id: int, ready, start, done: chan of int)
{
    ready <-= control->pid();
    <-start;
    for (round := 0; round < Rounds; round++) {
        at := (round + id) % len texts;
        value := real texts[at];
        check(value, at, "string to real");
        rendered := string value;
        check(real rendered, at, "real to string roundtrip");
        formatted := sys->sprint("%.17g", value);
        check(real formatted, at, "formatted precision roundtrip");
        scientific := sys->sprint("%.17e", value);
        check(real scientific, at, "scientific precision roundtrip");
    }
    done <-= control->pid();
}


# EN: Publish immutable normal/subnormal/extreme/signed-zero samples before spawn.
# Require exact bitwise results and complete child retirement, without VM quanta.
# RU: Опубликовать normal/subnormal/extreme/signed-zero случаи до spawn. Требовать
# точные биты и полное завершение детей, без квантов VM.
init()
{
    sys = load Sys Sys->PATH;
    math = load Math Math->PATH;
    control = load Processctl Processctl->PATH;
    texts = array[] of {"0.1", "3.141592653589793", "2.2250738585072014e-308",
        "1.7976931348623157e308", "4.9406564584124654e-324", "9007199254740991", "-0", "1"};
    expected = array[] of {big 16r3fb999999999999a, big 16r400921fb54442d18,
        big 16r0010000000000000, big 16r7fefffffffffffff, big 1,
        big 16r433fffffffffffff, big 16r8000000000000000, big 16r3ff0000000000000};
    ready := chan of int;
    start := chan of int;
    done := chan of int;
    for (id := 0; id < Workers; id++) {
        spawn worker(id, ready, start, done);
    }
    for (id = 0; id < Workers; id++) {
        <-ready;
    }
    for (id = 0; id < Workers; id++) {
        start <-= 1;
    }
    for (id = 0; id < Workers; id++) {
        pid := <-done;
        result := control->killwait(pid);
        if (result != Processctl->Retired && result != Processctl->Denied) {
            raise "decimal conversion resident did not retire";
        }
    }
    sys->print("PASS resident decimals workers=8 vectors=1024 exact_bits=1\n");
}
