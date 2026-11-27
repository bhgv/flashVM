/* Copyright (c) 2026 bhgv
 * EN: Scalar native 9P client boundary, using original Inferno codecs internally.
 * RU: Скалярная native-граница 9P client с исходными кодеками Inferno внутри.
 */
#ifndef STYX_CLIENT_H
#define STYX_CLIENT_H

/* EN/RU: Readable sections compile as one original translation unit; no runtime linkage is added.
 * Читаемые части компилируются как одна исходная translation unit; новой runtime-линковки нет. */
#include "readable_parts/client-h/part-001.inc"
#include "readable_parts/client-h/part-002.inc"
#include "readable_parts/client-h/part-003.inc"


/* EN: Send one Tcreate on a caller-owned unopened directory fid. Acknowledged
 * success opens the child on the same fid. Uncertainty fences the fid against retry.
 * RU: Передать один Tcreate на неоткрытом fid каталога вызывающего. При успехе
 * тот же fid открывает потомка. Неопределённый исход запрещает повтор на этом fid. */
int styxclientcreate(StyxFid *directory, const char *name, unsigned permissions, int mode, StyxTransfer *result);

/* EN: Create one directory with the given 0777 permission mask. Success opens
 * it for reading on the same fid. Serialize use and retirement of that fid.
 * RU: Создать каталог с правами в пределах 0777 и открыть для чтения на том же fid.
 * Сериализовать использование и закрытие fid. */
int styxclientmkdir(StyxFid *directory, const char *name, unsigned permissions, StyxTransfer *result);


/* EN: Send one Tremove and always consume fid. result receives the acknowledgement.
 * RU: Передать один Tremove и поглотить fid при любом исходе. Подтверждение в result. */
int styxclientremove(StyxFid *fid, StyxTransfer *result);

/* EN: Like create, with one optional stop token on a multiplexed client.
 * On success the caller still owns the same fid, now open on the new file.
 * RU: Как create, с необязательным токеном остановки мультиплексного клиента.
 * При успехе тот же fid принадлежит вызывающему и открыт на новом файле. */
int styxclientcreatecancel(StyxFid *directory, const char *name, unsigned permissions, int mode,
    StyxCancel *cancel, StyxTransfer *result);


/* EN: Own a single-use token borrowing stop/state through the synchronous
 * operation. The caller samples the hook while it waits, outside client locks.
 * Return NULL on allocation failure. Retire with styxcancelfree.
 * RU: Создать одноразовый токен, заимствующий stop/state на время синхронной
 * операции. Опрос идёт в вызывающем потоке вне замков клиента.
 * При нехватке памяти вернуть NULL. Освобождать через styxcancelfree. */
StyxCancel *styxcancelcreatepoll(StyxClientStop stop, void *state);


/* EN: Like mkdir with an optional one-use token. Keep the owned directory
 * fid until cleanup and read result to distinguish acknowledgement from uncertainty.
 * RU: Как mkdir с необязательным одноразовым токеном. Удерживать собственный
 * fid каталога до закрытия и проверять result на подтверждение или неопределённость. */
int styxclientmkdircancel(StyxFid *directory, const char *name, unsigned permissions,
    StyxCancel *cancel, StyxTransfer *result);


/* EN: Report whether a borrowed, pinned fid uses multiplexed cancellation.
 * Return zero for NULL or a serial transport. No I/O.
 * RU: Проверить поддержку мультиплексной отмены у удерживаемого fid.
 * Для NULL и последовательного транспорта вернуть ноль. Без ввода-вывода. */
int styxclientfidcancellable(StyxFid *fid);


/* EN: Like remove, with one optional stop token. Always consumes fid, scheduling
 * cleanup when the original request was not acknowledged.
 * RU: Как remove, с необязательным токеном остановки. Всегда поглощает fid.
 * Без ответа на исходный запрос планирует отдельное освобождение fid. */
int styxclientremovecancel(StyxFid *fid, StyxCancel *cancel, StyxTransfer *result);

#ifdef __cplusplus
}
#endif
#endif
