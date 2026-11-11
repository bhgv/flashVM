# Copyright (c) 2026 bhgv
# EN: Kill queued/admitted native owners, preserving cursor, roots and child replies.
# RU: Завершение ожидающих/допущенных владельцев с сохранением курсора и корней.
implement NativeKill;
include "sys.m";
include "processctl.m";

NativeKill: module { init: fn(); };
sys: Sys; # Immutable Sys interface published before spawn.
control: Processctl; # PID-scoped parent/child management.
DropOwner: con 0; # Driver can drop the parent FD while native child owns it.
Admitted: con 0; # Driver selects queued or already admitted native victims.
Rounds: con 8; # Complete native lifetimes per operation.


# EN: Announce identity, do one real operation, then report unexpected continuation.
# fd/op/ids/reports are captured; killwait must prevent the final report.
# RU: Сообщить PID, выполнить операцию, затем сообщить о лишнем продолжении.
# Входы захвачены; killwait обязан предотвратить последний отчёт.
victim(fd: ref Sys->FD, op: int, ids, reports: chan of int)
{
    ids <-= control->pid();
    bytes := array[1] of { * => byte 98 };
    if (op == 0) {
        sys->read(fd, bytes, 1);
    } else if (op == 1) {
        # Span multiple staging fragments; kill must stop the next admission.
        # Запросить несколько фрагментов; kill должен остановить следующий допуск.
        bytes = array[131072] of byte;
        sys->readn(fd, bytes, len bytes);
    } else if (op == 2) {
        sys->write(fd, bytes, 1);
    } else if (op == 3) {
        sys->fprint(fd, "b");
    } else if (op == 4) {
        sys->seek(fd, big 3, Sys->SEEKSTART);
    } else if (op == 5 || op == 6) {
        sys->fstat(fd);
    } else {
        sys->dirread(fd);
    }
    reports <-= 1;
}


# EN: Keep one real cursor operation admitted while its queued peer retires.
# ids identifies this separate head; done is sent only after genuine I/O completes.
# RU: Держать курсор за допущенной операцией, пока ожидающий сосед завершается.
# ids сообщает PID отдельной головы; done следует только за настоящим I/O.
head(fd: ref Sys->FD, directory: int, ids, done: chan of int)
{
    ids <-= control->pid();
    if (directory) {
        (n, entries) := sys->dirread(fd);
        if (n != 3 || len entries != 3) {
            raise "native kill changed the admitted directory head";
        }
    } else {
        if (sys->fprint(fd, "a") != 1) {
            raise "native kill changed the admitted file head";
        }
    }
    done <-= 1;
}


# EN: Confirm the host observed the exact native barrier, without scheduler quanta.
# input/permit belong to the controller; failure aborts the fixture.
# RU: Подтвердить наблюдение нативного барьера хостом без квантов VM.
# input/permit принадлежат контроллеру; ошибка завершает тест.
permit(input: ref Sys->FD, bytes: array of byte)
{
    if (sys->read(input, bytes, 1) != 1 || bytes[0] != byte 49) {
        raise "native kill barrier handshake changed";
    }
}


# EN: Require killwait retirement and same-handle recovery for all eight operations.
# Root the FD until its native victim has actually unwound; no foreign stack cleanup.
# RU: Проверить killwait и восстановление handle для всех восьми операций.
# Держать FD до настоящего выхода ребёнка из native, без очистки чужого стека.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    input := sys->fildes(0);
    bytes := array[1] of byte;
    for (round := 0; round < Rounds; round++) {
        for (op := 0; op < 8; op++) {
            directory := op >= 6;
            fd: ref Sys->FD;
            if (directory) {
                fd = sys->open("native-kill-dir", Sys->OREAD);
            } else if (Admitted && op != 2 && op != 3) {
                fd = sys->open("native-kill-input", Sys->OREAD);
            } else {
                fd = sys->create("native-kill-output-" + string (round * 8 + op), Sys->OWRITE, 8r666);
            }
            if (fd == nil) {
                raise sys->sprint("native kill descriptor setup failed: %r");
            }
            ids := chan of int;
            reports := chan[1] of int;
            done := chan of int;
            target: int;
            if (Admitted) {
                spawn victim(fd, op, ids, reports);
                target = <-ids;
                if (DropOwner) {
                    fd = nil;
                }
                permit(input, bytes);
            } else {
                spawn head(fd, directory, ids, done);
                <-ids;
                permit(input, bytes);
                spawn victim(fd, op, ids, reports);
                target = <-ids;
                permit(input, bytes);
            }
            if (control->killwait(target) != Processctl->Retired) {
                raise "native victim did not acknowledge retirement";
            }
            alt {
                <-reports => raise "native victim executed after its kill request";
                * => {}
            }
            if (control->kill(target) != Processctl->Denied ||
                    control->cancelwait(target) != Processctl->Denied ||
                    control->killwait(target) != Processctl->Denied) {
                raise "native victim accepted a request after retirement";
            }
            if (!Admitted) {
                sys->print("PASS queued native victim retired phase %d\n", round * 8 + op + 1);
                if (<-done != 1) {
                    raise "native head failed after queued victim retirement";
                }
            }
            if (DropOwner) {
                # Only the killed child's stack/native frame retained the FD.
                # FD удерживали только стек/нативный кадр завершённого ребёнка.
            } else if (directory) {
                (n, entries) := sys->dirread(fd);
                expected := 0;
                if (Admitted && op == 6) {
                    expected = 3;
                }
                if (n != expected || len entries != expected) {
                    raise "native directory cursor failed after killwait";
                }
            } else {
                position := big 1;
                if (Admitted && op == 1) {
                    position = big 6;
                } else if (Admitted && op == 4) {
                    position = big 3;
                } else if (Admitted && op == 5) {
                    position = big 0;
                }
                if (sys->seek(fd, big 0, Sys->SEEKRELA) != position) {
                    raise "native file cursor failed after killwait";
                }
            }
            fd = nil;
        }
    }
    sys->print("PASS resident native kill admitted=%d victims=64\n", Admitted);
}
