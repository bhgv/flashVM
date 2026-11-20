# Copyright (c) 2026 bhgv
# EN: Scalar dispatch over mounted Unibus request and reply files.
# RU: Скалярный вызов через смонтированные файлы запроса и ответа Unibus.
Styxscalar: module
{
    PATH: con "styxscalar.dis"; # Installed local transport helper / Локальный транспорт.
    call: fn(request, reply: string, requesttag, replytag: big, value: int): int;
};
