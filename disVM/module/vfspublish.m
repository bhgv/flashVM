# Copyright (c) 2026 bhgv
# EN: Pure Limbo copied publication requests over opaque native review capabilities.
# RU: Pure Limbo copied publication requests над opaque native review capabilities.
include "vfsexport.m";
Vfspublish: module
{
    PATH: con "vfspublish.dis"; # Convenience reader, native operations remain in Vfsexport.


    # EN/RU: Owner-created copied description; only handle authorizes answering.
    # Owner-created copied description; id/PID/name сами не дают authority.
    Request: adt
    {
        handle: ref Vfsexport->Request; # Opaque exact owner-side capability.
        id: big; # Review identity, never a redemption token.
        pid: int; # Native-captured actual local requester PID.
        kind: int; # Copied file/directory hint.
        rights: int; # Enforced requested access ceiling.
        name: string; # Copied UTF-8 logical name, not a host lookup.
    };


    # EN/RU: Load native module before sharing this library instance.
    # Загрузить native module до sharing library instance.
    init: fn();


    # EN/RU: Take and describe exactly one request; nil when none, raises malformed native input.
    # Получить ровно один request; nil если пусто, malformed input вызывает raise.
    next: fn(owner: ref Vfsexport->Owner): ref Request;


    # EN/RU: Answer with an ordinary chosen tree or nil/deny; fields cannot forge handle authority.
    # Ответить chosen tree либо nil/deny; copied fields не подделывают handle authority.
    reply: fn(owner: ref Vfsexport->Owner, request: ref Request, chosen: ref Vfsexport->Root): int;
};
