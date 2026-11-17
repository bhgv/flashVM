# Copyright (c) 2026 bhgv
# EN: Ordinary data stays terminal-first; actual module and transitive loads prefer provider.
# RU: Data остаётся terminal-first; настоящий module/transitive load предпочитает provider.
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


# EN/RU: Prime terminal image cache, load same virtual module, then remove provider and resolve again.
# Заполнить terminal image cache, загрузить same-name module, снять provider и повторить lookup.
init()
{
    sys := load Sys Sys->PATH;
    data := sys->open("/code/payload.dis", Sys->OREAD);
    bytes := array[1] of byte;
    check(data != nil && sys->read(data, bytes, 1) == 1 && string bytes == "T", "data order changed by suffix");
    first := load Dup "/terminal-code/dup.dis";
    check(first != nil && first->tag() == "T", "explicit terminal code grant");
    code := load Dup "/code/dup.dis";
    check(code != nil && code->tag() == "P" && code->dependent() == "P", "provider code/dependency priority or cache alias");
    check(sys->unmount("provider", "/code") == 0, "provider branch removal");
    next := load Dup "/code/dup.dis";
    check(next != nil && next->tag() == "T" && next->dependent() == "T" && code->tag() == "P",
        "absence fallback, frozen loaded code or source cache identity");
    sys->print("PASS provider-first code and terminal-first data with dependencies\n");
}
