# Copyright (c) 2026 bhgv
# EN: Test rendezvous forcing one caller-side write after the provider receives a snapshot.
# RU: Тестовая синхронизация записи клиента после получения снимка провайдером.


# EN: Wait for a byte in a driver-provided marker file, with a bounded poll.
# RU: Дождаться байта в файле-маркере драйвера с ограниченным опросом.
wait_marker(sys: Sys, path: string)
{
    state := array[1] of byte;
    for (tries := 0; tries < 180; tries++) {
        fd := sys->open(path, Sys->OREAD);
        if (fd == nil) {
            raise "could not open ref pair test marker";
        }
        length := sys->read(fd, state, 1);
        fd = nil;
        if (length < 0) {
            raise "could not read ref pair test marker";
        }
        if (length == 1 && state[0] == byte '1') {
            return;
        }
        sys->sleep(10);
    }
    raise "ref pair test marker timed out";
}


# EN: Write the marker byte after the corresponding operation is complete.
# RU: Записать байт-маркер после завершения соответствующей операции.
set_marker(sys: Sys, path: string)
{
    fd := sys->open(path, Sys->OWRITE);
    value := array[] of {byte '1'};
    if (fd == nil || sys->write(fd, value, 1) != 1) {
        raise "could not set ref pair test marker";
    }
}
