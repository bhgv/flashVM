/* Copyright (c) 2026 bhgv
 * EN: Scalar native single-file publication fixture boundary, independent of Inferno typedefs.
 * RU: Scalar native boundary теста single-file publication, отдельно от Inferno typedefs.
 */
#ifndef FILE_ROOT_BACKEND_H
#define FILE_ROOT_BACKEND_H
#include "peer.h"


/* EN/RU: Create direct/deferred single-file publication; nonzero prepared starts native reviewer.
 * Создать direct/deferred single-file publication; prepared запускает native reviewer. */
void filerootsetup(int prepared);


/* EN/RU: Return owned locally preauthorized peer/NULL; wire uname is never trusted identity.
 * Вернуть свой locally preauthorized peer/NULL; wire uname не trusted identity. */
StyxPeer *filerootpeer(void);


/* EN/RU: Join reviewer and consume publication/provider after socket owner retires all fids.
 * Join reviewer и поглотить publication/provider после retirement всех fids socket owner. */
void filerootend(void);
#endif
