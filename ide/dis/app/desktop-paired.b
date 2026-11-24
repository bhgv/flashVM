# Copyright (c) 2026 bhgv
# EN: Upload a presentation SWF on the mounted session, then enter the stock desktop.
# RU: Загрузить SWF представления через смонтированную сессию и запустить штатный desktop.

implement PairedDesktop;
include "sys.m";
include "desktop.m";

PairedDesktop: module {
    init: fn(); # Runs on the owner that later registers the desktop channels.
};


# EN: Copy a counted SWF in bounded writes. Both descriptors retire on return.
# RU: Передать SWF с длиной ограниченными порциями. По возврате дескрипторы освобождаются.
uploadmovie(sys: Sys)
{
    source := sys->open("desktop-view.swf", Sys->OREAD);
    if (source == nil) {
        raise sys->sprint("open desktop-view.swf: %r");
    }
    (status, info) := sys->fstat(source);
    if (status < 0 || info.length <= big 0 || info.length > big 67108864) {
        raise "desktop-view.swf must be between 1 byte and 64 MiB";
    }
    size := int info.length;
    upload := sys->open("/rpc/.upload", Sys->OWRITE);
    if (upload == nil) {
        raise sys->sprint("open desktop .upload: %r");
    }
    count := array[4] of {byte (size >> 24), byte (size >> 16), byte (size >> 8), byte size};
    if (sys->write(upload, count, len count) != len count) {
        raise sys->sprint("write desktop SWF length: %r");
    }
    buffer := array[4096] of byte;
    while (size > 0) {
        chunk := size;
        if (chunk > len buffer) {
            chunk = len buffer;
        }
        if (sys->readn(source, buffer, chunk) != chunk) {
            raise sys->sprint("read desktop-view.swf: %r");
        }
        # A remote write can end at the negotiated Styx message boundary.
        # Удалённая запись может завершиться на границе сообщения Styx.
        for (offset := 0; offset < chunk;) {
            written := sys->write(upload, buffer[offset:chunk], chunk - offset);
            if (written <= 0) {
                raise sys->sprint("write desktop-view.swf: %r");
            }
            offset += written;
        }
        size -= chunk;
    }
}


# EN: Start the player through its service files, then hand control to DesktopIDE.
# RU: Запустить плеер через служебные файлы и передать управление DesktopIDE.
init()
{
    sys := load Sys Sys->PATH;
    desktop := load DesktopIDE DesktopIDE->PATH;
    if (desktop == nil) {
        raise sys->sprint("load desktop-ide.dis: %r");
    }
    uploadmovie(sys);
    state := sys->open("/rpc/.state", Sys->OWRITE);
    if (state == nil || sys->write(state, array of byte "start", 5) != 5) {
        raise sys->sprint("start desktop-view.swf: %r");
    }
    state = nil;
    desktop->init();
}
