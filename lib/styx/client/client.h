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

#ifdef __cplusplus
}
#endif
#endif
