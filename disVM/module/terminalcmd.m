# Copyright (c) 2026 bhgv
# EN: Optional generic application entry receiving FlashVM channels explicitly.
# RU: Необязательный общий entry приложения с explicit каналами FlashVM.
include "terminal.m";
Terminalcmd: module
{


    # EN/RU: Run on the provider VM with prepared environment, borrowed terminal and argument list.
    # Выполнить на provider VM в prepared environment с borrowed terminal/args; errors propagate.
    # EN/RU: terminal may be nil for console applications; inherited Sys FD 0/1/2 remain usable.
    # Для консольных приложений terminal может быть nil; унаследованные Sys FD 0/1/2 доступны.
    init: fn(terminal: ref Terminal->Session, args: list of string);
};
