# Copyright (c) 2026 bhgv
# EN: One trusted network connection served by its own rooted permanent object owner.
# RU: Одно доверенное сетевое соединение обслуживается отдельным rooted permanent owner.
Proxyserver: module
{
    PATH: con "proxyserver.dis"; # Separate bridge coordinator and owner execution.


    # EN/RU: Load immutable dependencies before serving; no connection is opened here.
    # Загрузить immutable dependencies до serve; соединение здесь не открывается.
    init: fn();


    # EN: Serve typed variable/function exports over exclusively borrowed input/
    # output directions. high/low must be externally unique and never reused.
    # Fixed frame budget/timeout bounds native IO; one exact completed request is
    # cached for duplicate replay. Always killwait the original child owner on exit.
    # RU: Обслуживать typed exports через исключительно borrowed input/output.
    # high/low уникальны извне и не переиспользуются. Fixed frame/timeout ограничивают
    # IO; один completed request кешируется для duplicate replay. При выходе всегда
    # killwait исходного child owner. Return status describes connection termination.
    serve: fn(input, output: ref Sys->FD, high, low: big, timeout: int,
              handler: Proxyhandler, cell, capture: ref Proxyhost->Cell): int;
};
