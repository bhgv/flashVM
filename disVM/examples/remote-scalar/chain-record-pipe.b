# Copyright (c) 2026 bhgv
# EN: Share the two fixed S-record fixtures and read a virtual rendezvous file.
# RU: Общие тестовые S-записи и чтение виртуального файла обмена.


# EN: Construct one canonical S-record with tag 1 and a four-byte test integer.
# RU: Собрать каноническую S-запись с тегом 1 и четырёхбайтовым тестовым числом.
pipe_test_record(reply: int): array of byte
{
    if (reply) {
        return array[] of {byte 158, byte 133, byte 129, byte 0, byte 0, byte 0, byte 73};
    }
    return array[] of {byte 188, byte 133, byte 129, byte 0, byte 0, byte 0, byte 42};
}


# EN: Wait for one complete record or receipt, with a finite test deadline.
# RU: Дождаться одной полной записи или подтверждения за конечное тестовое время.
pipe_read(sys: Sys, file: ref Sys->FD, capacity: int): array of byte
{
    data := array[capacity] of byte;
    for (attempt := 0; attempt < 500; attempt++) {
        n := sys->read(file, data, len data);
        if (n < 0) {
            raise sys->sprint("record pipe read: %r");
        }
        if (n > 0) {
            return data[:n];
        }
        sys->sleep(10);
    }
    raise "record pipe receive timed out";
}


# EN: Check the full record bytes, including the framing and checksum.
# RU: Проверить все байты записи, включая рамку и контрольную сумму.
pipe_check(actual, expected: array of byte)
{
    if (len actual != len expected) {
        raise "record pipe length mismatch";
    }
    for (i := 0; i < len actual; i++) {
        if (actual[i] != expected[i]) {
            raise "record pipe value mismatch";
        }
    }
}


# EN: Send a receipt on the receiving fid after the local consumer accepts.
# RU: Передать подтверждение по fid получателя после приёма локальным потребителем.
pipe_accept(sys: Sys, file: ref Sys->FD)
{
    received := array[] of {byte 1};
    if (sys->write(file, received, len received) != len received) {
        raise sys->sprint("record pipe receipt: %r");
    }
}
