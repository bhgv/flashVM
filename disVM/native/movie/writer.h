/* Copyright (c) 2026 bhgv
 * EN: One native I/O owner's automatic, prefix-preserving movie announcement writer.
 * RU: Автоматический movie announcement writer одного native I/O owner с сохранением prefix.
 */
#ifndef STYX_MOVIE_WRITER_H
#define STYX_MOVIE_WRITER_H
#include "publisher.h"
typedef struct StyxMovieWriter StyxMovieWriter; /* Owned native serializer; one exclusive I/O caller. */


/* EN/RU: Borrow output bytes until callback returns; initialize complete precise acknowledgement.
 * No guest pointers. Called once per step on I/O owner, outside source/queue locks.
 * Bytes borrowed до return; задать точное подтверждение. Один вызов за step на I/O owner. */
typedef void (*StyxMovieWrite)(void *state, const void *bytes, size_t size, DVM_IoResult *out);


/* EN/RU: Borrow trusted source/reserved tag; owned serializer/NULL, initial subscription automatic.
 * No transport I/O or new authority. Retains source independently through native outbox.
 * Source/tag borrowed; свой serializer/NULL, автоматическая подписка, без I/O/new authority. */
StyxMovieWriter *styxmoviewriter(DVM_MovieOffer *source, unsigned long tag);


/* EN/RU: Attempt at most one native write; borrowed writer/callback/state, precise result by value.
 * Preserve unsent tail across partial success/AGAIN; EMPTY returns AGAIN without callback.
 * CLOSED is fully sent before sticky EOF. Uncertain/malformed/failing acknowledgements halt
 * the writer permanently; a new transport must use a new authorized connection, never replay a tail.
 * Caller serializes with all other control writers through the whole pending record pair.
 * Не более одного native write; сохранить tail при partial/AGAIN. CLOSED перед EOF.
 * Uncertain/malformed/failure навсегда останавливают writer; межпоточное использование запрещено.
 * Caller не перемежает bytes других control records до завершения pending пары. */
DVM_IoResult styxmoviewriterstep(StyxMovieWriter *writer, StyxMovieWrite write, void *state);


/* EN/RU: Borrow readiness fd, or -1 for invalid/ended/failed writer; never read/close it.
 * When pending()!=0, watch transport writability instead; source fd need not remain signalled.
 * Borrow readiness fd/-1; при pending()!=0 ждать transport writability, fd не читать/закрывать. */
int styxmoviewriterfd(StyxMovieWriter *writer);


/* EN/RU: Return unsent staged bytes/0; one owner only, no I/O. Borrow writer.
 * Число staged unsent bytes/0; writer borrowed, один owner, без I/O. */
size_t styxmoviewriterpending(StyxMovieWriter *writer);


/* EN/RU: Consume after I/O caller retires; detach without joining source processes/callbacks.
 * Поглотить после завершения I/O caller; detach без ожидания source processes/callbacks. */
void styxmoviewriterfree(StyxMovieWriter *writer);
#endif
