# Copyright (c) 2026 bhgv
# EN: Ordinary bootstrap loads a console module and invokes the existing init interface with nil UI.
# RU: Обычный bootstrap вызывает прежний init консольного модуля с nil UI.
implement Check;
include "terminalcmd.m";
Check: module { init: fn(); };


# EN/RU: Invoke the real typed application entry without constructing a terminal session.
# Вызвать настоящий typed entry приложения без создания terminal session.
init()
{
    app := load Terminalcmd "./console.dis";
    if (app == nil) {
        raise "cannot load console application";
    }
    app->init(nil, nil);
}
