# Copyright (c) 2026 bhgv
# EN: Example client for owner-side command execution through a mounted namespace.
# RU: Пример клиента запуска на стороне владельца через смонтированное пространство имён.
Runexample: module
{
    PATH: con "/runexample.dis"; # Example binding in the TCP fixture.

    # EN/RU: Submit command and payload routes and return the completed command's exit status.
    # Отправить команду и двоичные данные по заданным маршрутам, вернуть код завершения команды.
    execute: fn(command, payloadroute, result: string, payload: array of byte): int;
};
