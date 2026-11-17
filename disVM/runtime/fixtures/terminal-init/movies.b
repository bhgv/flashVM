# Copyright (c) 2026 bhgv
# EN: Real Sys FD requests to the automatically prepared VFS service, with independent wire oracles.
# RU: Настоящие Sys FD requests к автоматически prepared VFS service и independent wire oracles.
implement Moviecheck;
include "terminal.m";
include "moviecheck.m";
include "movieframes.m";


# EN/RU: Fail an explicit integration contract in interpreter/JIT, without optimized-away assertions.
# Явно завершить нарушение integration contract в interpreter/JIT.
check(ok: int, message: string)
{
    if (!ok) {
        raise message;
    }
}


# EN/RU: Decode fixture ASCII hex into exact native protocol bytes; inputs borrowed, new array owned.
# Декодировать ASCII hex fixture в точные protocol bytes; inputs borrowed, новый array owned.
unhex(text: string): array of byte
{
    digits := "0123456789abcdef";
    out := array[len text / 2] of byte;
    for (i := 0; i < len out; i++) {
        high := -1;
        low := -1;
        for (j := 0; j < len digits; j++) {
            if (text[i * 2] == digits[j]) {
                high = j;
            }
            if (text[i * 2 + 1] == digits[j]) {
                low = j;
            }
        }
        check(high >= 0 && low >= 0, "bad fixture hex");
        out[i] = byte (high * 16 + low);
    }
    return out;
}


# EN/RU: Open one ordinary FD, write coalesced declaration/request and verify exact response bytes.
# Live sources close after one chunk; the finite NEWNS SWF includes its successful END.
# Открыть FD, передать declaration/request и проверить bytes; NEWNS SWF включает END.
fetch(term: Terminal, session: ref Terminal->Session, request, expected: string)
{
    sys := load Sys Sys->PATH;
    fd := term->movies(session);
    check(fd != nil, "automatic movie service FD missing");
    bytes := unhex(request);
    check(sys->write(fd, bytes, len bytes) == len bytes, "movie request write");
    reply := unhex(expected);
    actual := array[len reply] of byte;
    check(sys->readn(fd, actual, len actual) == len actual, "movie response prefix");
    for (i := 0; i < len reply; i++) {
        check(actual[i] == reply[i], "movie code/data record mismatch");
    }
}


# EN/RU: Typed init session already contains an exact separate resource grant, before application work.
# Typed init session уже содержит separate resource grant до работы приложения.
run(term: Terminal, session: ref Terminal->Session)
{
    sys := load Sys Sys->PATH;
    check(session != nil && session.movie != nil && term->movies(nil) == nil, "movie launch capability");
    fetch(term, session, CodeRequest, CodeReply);
    fetch(term, session, DataRequest, DataReply);
    sys->print("PASS automatic terminal movie code/data service\n");
}


# EN/RU: Child gets a fresh service on FORKNS; explicit older grants retain their original namespace.
# Child получает fresh service при FORKNS; старый explicit grant удерживает исходный namespace.
scope(term: Terminal, session: ref Terminal->Session)
{
    sys := load Sys Sys->PATH;
    check(sys->pctl(Sys->FORKNS, nil) == -1, "movie scope refusal injection");
    fetch(term, term->current(), CodeRequest, CodeReply);
    check(sys->pctl(Sys->FORKNS, nil) > 0, "movie child namespace fork");
    changed := term->current();
    check(changed != nil && changed.movie != nil && sys->unmount(nil, "/") == 0, "movie scope setup");
    fetch(term, changed, CodeRequest, DeniedReply);
    fetch(term, session, CodeRequest, CodeReply);
    sys->print("PASS terminal movie child namespace and retained grant\n");
    check(sys->chdir("/pkg") == 0 && sys->pctl(Sys->NEWNS, nil) > 0, "movie NEWNS package root");
    fresh := term->current();
    check(fresh != nil && fresh.movie != nil && sys->open("/etc/passwd", Sys->OREAD) == nil,
        "movie NEWNS authority");
    fetch(term, fresh, FreshRequest, FreshReply);
    fetch(term, fresh, CodeRequest, DeniedReply);
    sys->print("PASS terminal movie fresh NEWNS SWF stream\n");
}


# EN/RU: Live removal affects fresh requests through an existing grant; control channels stay granted.
# Live removal влияет на новые requests existing grant; control channels сохраняют grant.
denied(term: Terminal, session: ref Terminal->Session)
{
    fetch(term, session, CodeRequest, DeniedReply);
}
