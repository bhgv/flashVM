# Copyright (c) 2026 bhgv
# EN: Verify binary launch arguments received from the native C client.
# RU: Проверить стартовые аргументы от нативного C-клиента.
implement Program;
include "launchcontext.m";

Program: module { init: fn(); };


# EN: The provider should return a failed launch for any other payload.
# RU: Провайдер возвращает неуспешный запуск для любых других данных.
init()
{
    context := load Launchcontext Launchcontext->PATH;
    if (context == nil || string context->arguments() != "native-c") {
        raise "wrong startup payload";
    }
}
