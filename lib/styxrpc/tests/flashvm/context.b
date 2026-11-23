# Copyright (c) 2026 bhgv
# EN: Verify FlashVM-launched startup bytes and inherited reverse resources.
# RU: Проверка стартовых байтов запуска FlashVM и встречных ресурсов.
implement Context;
include "sys.m";
include "launchcontext.m";

Context: module { init: fn(); };


# EN: Observe one fixed reverse context and update its shared variable and FD.
# RU: Прочитать фиксированный встречный контекст и обновить переменную и FD.
init()
{
    sys := load Sys Sys->PATH;
    launch := load Launchcontext Launchcontext->PATH;
    args := launch->arguments();
    if (string args == "park") {
        checkpoint := sys->open("/.vfs/parked", Sys->OWRITE);
        if (checkpoint == nil || sys->write(checkpoint, array[] of {byte 'P'}, 1) != 1) {
            raise "missing cancellation checkpoint";
        }
        gate := chan of int;
        <-gate;
        raise "parked launch unexpectedly resumed";
    }
    if (len args != 3 || args[0] != byte 'A' || args[1] != byte 0 || args[2] != byte 'B') {
        raise "wrong counted startup bytes";
    }
    marker := sys->open("/.vfs/marker", Sys->OREAD);
    input := sys->open("/.prog/.self/.var/count", Sys->OREAD);
    update := sys->open("/.prog/.self/.var/count", Sys->OWRITE);
    output := sys->fildes(7);
    data := array[4] of byte;
    if (marker == nil || input == nil || update == nil || output == nil ||
        sys->read(marker, data, 1) != 1 || data[0] != byte 'C' ||
        sys->read(input, data, 4) != 4 || data[0] != byte 0 || data[1] != byte 0 ||
        data[2] != byte 0 || data[3] != byte 41) {
        raise sys->sprint("wrong reverse context: %r");
    }
    data[3] = byte 42;
    if (sys->write(update, data, 4) != 4 || sys->write(output, array[] of {byte 'D'}, 1) != 1) {
        raise sys->sprint("wrong reverse write: %r");
    }
}
