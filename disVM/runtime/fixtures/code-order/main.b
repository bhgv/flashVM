# Copyright (c) 2026 bhgv
# EN: Ordinary data stays terminal-first; module and transitive loads prefer their own side, then fall through.
# RU: Данные идут по стеку; модуль и зависимости выбираются сначала со своей стороны.
implement Check;
include "sys.m";
include "dup.m";
Check: module { init: fn(); };


# EN/RU: Raise an observable interpreter/JIT integration mismatch.
# Явно завершить interpreter/JIT integration mismatch.
check(ok: int, message: string)
{
    if (!ok) {
        raise message;
    }
}


# EN/RU: Prime provider image cache, load the terminal owner's same virtual name, then remove its layer and resolve again.
# RU: Заполнить кэш провайдерским модулем, загрузить одноимённый со своей стороны, снять свой слой и повторить поиск.
init()
{
    sys := load Sys Sys->PATH;
    data := sys->open("/code/payload.dis", Sys->OREAD);
    bytes := array[1] of byte;
    check(data != nil && sys->read(data, bytes, 1) == 1 && string bytes == "T", "data order changed by suffix");
    first := load Dup "/provider-code/dup.dis";
    check(first != nil && first->tag() == "P", "explicit provider code grant");
    code := load Dup "/code/dup.dis";
    check(code != nil && code->tag() == "T" && code->dependent() == "T", "own-side code/dependency priority or cache alias");
    check(sys->unmount("terminal", "/code") == 0, "own-side branch removal");
    next := load Dup "/code/dup.dis";
    check(next != nil && next->tag() == "P" && next->dependent() == "P" && code->tag() == "T",
        "absence fallback, frozen loaded code or source cache identity");
    sys->print("PASS own-side-first code and terminal-first data with dependencies\n");
}
