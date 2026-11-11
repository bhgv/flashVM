# Copyright (c) 2026 bhgv
# EN: Cancel eight queued native operations without disturbing the admitted owner.
# RU: Отмена восьми ожидающих нативных операций без нарушения активного владельца.
implement NativeCancel;
include "sys.m";

NativeCancel: module { init: fn(); };
sys: Sys; # Immutable builtin interface initialized before spawn.
Rounds: con 8; # Repeated complete file/directory cancellation sequences.


# EN: Hold the real native cursor until the private host releases its I/O barrier.
# fd is rooted until done; directory selects a three-entry directory batch.
# RU: Держать настоящий курсор до снятия частного нативного барьера.
# fd укоренён до done; directory выбирает чтение трёх записей каталога.
head(fd: ref Sys->FD, directory: int, done: chan of int)
{
    if (directory) {
        (n, entries) := sys->dirread(fd);
        if (n != 3 || len entries != 3) {
            raise "admitted directory operation changed";
        }
    } else {
        if (sys->fprint(fd, "a") != 1) {
            raise "admitted file operation changed";
        }
    }
    done <-= 1;
}


# EN: Attempt one queued operation; require cancellation and unchanged result data.
# fd/op are captured inputs, done acknowledges only after the error is checked.
# RU: Выполнить ожидающую операцию; проверить отмену и неизменность данных.
# fd/op захвачены при spawn; done подтверждает завершение после проверки ошибки.
tail(fd: ref Sys->FD, op: int, done: chan of int)
{
    bytes := array[1] of { * => byte 90 };
    expected: string;
    if (op == 0 || op == 1) {
        n := 0;
        if (op == 0) {
            n = sys->read(fd, bytes, 1);
        } else {
            n = sys->readn(fd, bytes, 1);
        }
        if (n != -1 || bytes[0] != byte 90) {
            raise "cancelled input changed its destination";
        }
        expected = "native input cancelled";
    } else if (op == 2) {
        if (sys->write(fd, bytes, 1) != -1) {
            raise "cancelled write was reported as committed";
        }
        expected = "native output cancelled";
    } else if (op == 3) {
        if (sys->fprint(fd, "x") != -1) {
            raise "cancelled formatted write was reported as committed";
        }
        expected = "native formatted output cancelled";
    } else if (op == 4) {
        if (sys->seek(fd, big 1024, Sys->SEEKSTART) != big -1) {
            raise "cancelled seek changed its result";
        }
        expected = "native seek cancelled";
    } else if (op == 5 || op == 6) {
        (n, entry) := sys->fstat(fd);
        if (n != -1 || entry.name != nil || entry.length != big 0) {
            raise "cancelled metadata published a record";
        }
        expected = "native metadata cancelled";
    } else {
        (n, entries) := sys->dirread(fd);
        if (n != -1 || entries != nil) {
            raise "cancelled directory read published a batch";
        }
        expected = "native directory read cancelled";
    }
    if (sys->sprint("%r") != expected) {
        raise "native cancellation reason was lost";
    }
    done <-= 1;
}


# EN: Spawn one permanent owner per operation; the host confirms the head barrier.
# Verify same-handle recovery and cursor bytes after each cancellation.
# RU: Создать постоянного владельца операции; хост подтверждает барьер головы.
# Проверить восстановление того же handle и байты курсора после каждой отмены.
init()
{
    sys = load Sys Sys->PATH;
    file := sys->create("native-cancel.bin", Sys->OWRITE, 8r666);
    input := sys->fildes(0);
    permit := array[1] of byte;
    if (file == nil || input == nil) {
        raise "native cancellation setup failed";
    }
    for (round := 0; round < Rounds; round++) {
        for (op := 0; op < 8; op++) {
            fd := file;
            directory := op >= 6;
            if (directory) {
                fd = sys->open("native-cancel-dir", Sys->OREAD);
                if (fd == nil) {
                    raise "native directory cancellation setup failed";
                }
            }
            done := chan of int;
            spawn head(fd, directory, done);
            if (sys->read(input, permit, 1) != 1 || permit[0] != byte 49) {
                raise "native head barrier was not confirmed";
            }
            spawn tail(fd, op, done);
            if (<-done != 1 || <-done != 1) {
                raise "native cancellation completion changed";
            }
            if (directory) {
                (n, entries) := sys->dirread(fd);
                if (n != 0 || entries != nil) {
                    raise "directory cursor did not recover after cancellation";
                }
            } else {
                expected := big (round * 7 + op + 1);
                (n, info) := sys->fstat(file);
                if (sys->seek(file, big 0, Sys->SEEKRELA) != expected || n != 0 || info.length != expected) {
                    raise "file cursor did not recover after cancellation";
                }
            }
            fd = nil;
        }
        if (sys->fprint(file, "b") != 1) {
            raise "file did not accept output after cancellation sequence";
        }
    }
    sys->print("PASS resident native cancellation operations=64 owners=128\n");
}
