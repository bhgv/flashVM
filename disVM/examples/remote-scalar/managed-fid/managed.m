# Copyright (c) 2026 bhgv
# EN: Interface shared by the ordinary client and the original remote implementation.
# RU: Общий интерфейс обычного клиента и исходной удалённой реализации.
Managed: module {
    echo: fn(text: string, bytes: array of byte): (string, array of byte);
    accept: fn(text: string);
};
