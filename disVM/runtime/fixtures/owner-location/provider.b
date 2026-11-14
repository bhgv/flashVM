# Copyright (c) 2026 bhgv
# EN: Keep an imported module current while the owner blocks on an ordinary channel.
# RU: Сохранить imported module текущим, пока owner ждёт на обычном канале.
implement LocationProvider;
include "provider.m";


# EN: Borrow ready/gate; tell parent the callee is active, then return its exact payload.
# RU: Заимствовать ready/gate; сообщить parent о входе в callee, вернуть точный payload.
wait(ready, gate: chan of int): int
{
    ready <-= 1;
    value := <-gate;
    return value + 7;
}
