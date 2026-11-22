# Copyright (c) 2026 bhgv
# EN: Check the exact values seen by the original export.
# RU: Проверить значения, полученные исходным экспортом.
implement Target;
Target: module {
    Cell: adt { narrow: int; wide: big; };
    update: fn(a, b: ref Cell): big;
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


# EN: Update through both arguments to check reference identity.
# RU: Изменить объект через оба аргумента для проверки идентичности ссылки.
update(a, b: ref Cell): big
{
    a.narrow++;
    b.wide += big b.narrow;
    return a.wide;
}
