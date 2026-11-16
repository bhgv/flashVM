/* Copyright (c) 2026 bhgv
 * EN: Private native fd multiplexer boundary; original codecs remain authoritative.
 * RU: Private native fd multiplexer; исходные codecs остаются authoritative.
 */
#ifndef STYX_MUX_H
#define STYX_MUX_H
#include "client.h"
#include "life.h"
typedef struct StyxMux StyxMux; /* Refcounted autonomous native socket owner. */
typedef struct StyxFlight StyxFlight; /* One bounded admitted transaction. */
typedef struct StyxAnswer
{
    unsigned char *bytes; /* Owned raw original response frame, NULL without acknowledgement. */
    unsigned length; /* Complete raw response length. */
    int attempted; /* Original frame started sending. */
    int cancelled; /* Cancel/timeout retired without original acknowledgement. */
    int status; /* 1 expected response,0 Rerror,-1 no acknowledgement. */
} StyxAnswer;

/* EN/RU: Consume connected fd always; owned native engine/NULL. POSIX only.
 * Принять connected fd всегда; свой native engine/NULL. */
StyxMux *styxmuxcreate(int fd, unsigned capacity, unsigned timeout, unsigned limit);


/* EN/RU: Borrow terminal source while retaining engine owner. Без передачи владения. */
StyxLife *styxmuxlife(StyxMux *mux);


/* EN/RU: Admit copied frame within quota; caller owns flight/NULL. Client lock held.
 * Принять copied frame в quota; свой flight/NULL, client lock удержан. */
StyxFlight *styxmuxsubmit(StyxMux *, const void *, unsigned, unsigned, StyxCancel *, int);
/* EN/RU: Collect owned raw response after native completion; consumes flight.
 * Получить свой raw response после native completion; поглощает flight. */
StyxAnswer styxmuxcollect(StyxFlight *);
/* EN/RU: Local native engine state snapshot, no network I/O.
 * Local native engine state snapshot, без network I/O. */
int styxmuxalive(StyxMux *);
/* EN/RU: Terminally reject all work and wake native owner; no guest access.
 * Terminally отклонить всю работу и разбудить owner, без guest access. */
void styxmuxfail(StyxMux *);
/* EN/RU: Explicit native shutdown waits for fd owner retirement; retains engine.
 * Явный native shutdown ждёт fd owner retirement, engine не поглощает. */
void styxmuxshutdown(StyxMux *);
/* EN/RU: Consume owner reference without joining; autonomous worker retires itself.
 * Поглотить owner ref без join; worker завершает себя сам. */
void styxmuxfree(StyxMux *);
/* EN/RU: Nonwaiting pipe signal while holding engine owner or bound cancel token.
 * Сигнал pipe без ожидания при owner ref либо bound cancel token. */
void styxmuxwake(StyxMux *);
/* EN/RU: Cancel binding retains token until unbind; only one admitted flight per token, never reuse.
 * Bind удерживает token до unbind; только один flight на token. */
int styxcancelbind(StyxCancel *, StyxMux *);
int styxcancelled(StyxCancel *);
void styxcancelunbind(StyxCancel *);


/* EN/RU: Reserve an unused borrowed outer token for one logical fragmented walk.
 * Own a scope pin on success1;0 rejects reuse/overflow, NULL succeeds without pin.
 * Зарезервировать unused outer token на fragmented walk;1 и scope pin/0 отказ. */
int styxcancelbegin(StyxCancel *cancel);


/* EN/RU: Borrow reserved outer scope, replace completed fragment with fresh child/NULL.
 * Child is scope-owned until next step/end; sticky request forwards atomically.
 * NULL is OOM, not permission to issue an uncancellable RPC. No guest state.
 * Заменить завершённый fragment на fresh child/NULL; child принадлежит scope. */
StyxCancel *styxcancelstep(StyxCancel *cancel);


/* EN/RU: Consume successful scope pin after all fragment RPCs retired; NULL harmless.
 * Поглотить scope pin после retirement всех fragment RPC; NULL допустим. */
void styxcancelend(StyxCancel *cancel);


/* EN/RU: Configure borrowed live engine heartbeat without waiting;1/0.
 * Настроить heartbeat удержанного engine без ожидания;1/0. */
int styxmuxkeepalive(StyxMux *, unsigned, unsigned, unsigned);


/* EN/RU: Local streaming capability requires live fd owner with at least two RPC slots.
 * Local streaming capability требует live fd owner минимум с двумя RPC slots. */
int styxmuxwatchable(StyxMux *mux);
#endif
