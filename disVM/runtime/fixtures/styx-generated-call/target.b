# Copyright (c) 2026 bhgv
# EN: Check the exact values seen by the original export.
# RU: Проверить значения, полученные исходным экспортом.
implement Target;
Target: module {
    invoke: fn(x: byte, s: string, a: array of byte, n: big): string;
};

# EN: Verify all argument slots before forming the result.
# RU: Проверить все слоты аргументов перед формированием результата.
invoke(x: byte, s: string, a: array of byte, n: big): string
{
    if (x != byte 23 || a == nil || len a != 1 || a[0] != byte 11 || n != big 77) {
        raise "synthesized call argument mismatch";
    }
    return s + "!";
}
