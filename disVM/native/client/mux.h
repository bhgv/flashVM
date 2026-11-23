/* Copyright (c) 2026 bhgv
 * EN: Private native carrier multiplexer boundary; original codecs remain authoritative.
 * RU: Private native carrier multiplexer; исходные codecs остаются authoritative.
 */
#ifndef STYX_MUX_H
#define STYX_MUX_H
#include "client.h"
#include "life.h"
typedef struct StyxMux StyxMux; /* Refcounted autonomous native carrier owner. */
typedef struct StyxFlight StyxFlight; /* One bounded admitted transaction. */
typedef struct StyxAnswer
{
    unsigned char *bytes; /* Owned raw original response frame, NULL without acknowledgement. */
    unsigned length; /* Complete raw response length. */
    int attempted; /* Original frame started sending. */
    int cancelled; /* Cancel/timeout retired without original acknowledgement. */
    int cycle; /* Proven negotiated remote traversal loop, never inferred from Rerror text. */
    int status; /* 1 expected response,0 Rerror,-1 no acknowledgement. */
} StyxAnswer;

/* EN/RU: Consume connected reliable ordered stream PHY always; owned native engine/NULL.
 * Принять connected stream PHY всегда; свой native engine/NULL. */
StyxMux *styxmuxcreatephy(RemotePhy *phy, unsigned capacity, unsigned timeout, unsigned limit);

/* EN/RU: Explicit history profile, consumes PHY on every outcome; same bounds, nonzero direction-local data tag.
 * Явный history profile; PHY принят всегда, прежние bounds, nonzero data tag.
 */
StyxMux *styxmuxcreatewalkphy(RemotePhy *phy, unsigned capacity, unsigned timeout, unsigned limit, unsigned long tag);


/* EN/RU: Borrow terminal source while retaining engine owner. Без передачи владения. */
StyxLife *styxmuxlife(StyxMux *mux);


/* EN: Admit a copied frame within quota and return an owned flight or NULL.
 * Clunks and opt-in tokens may wait for an initial tag with the client lock held.
 * RU: Принять копию кадра в пределах квоты и вернуть свой flight или NULL.
 * Tclunk и токены ожидания могут ждать начального тега под замком клиента. */
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
/* EN/RU: Explicit native shutdown waits for carrier owner retirement; retains engine.
 * Явный native shutdown ждёт carrier owner retirement, engine не поглощает. */
void styxmuxshutdown(StyxMux *);
/* EN/RU: Consume owner reference without joining; autonomous worker retires itself.
 * Поглотить owner ref без join; worker завершает себя сам. */
void styxmuxfree(StyxMux *);
/* EN/RU: Nonwaiting PHY interrupt while holding engine owner or bound cancel token.
 * Сигнал PHY без ожидания при owner ref либо bound cancel token. */
void styxmuxwake(StyxMux *);
/* EN/RU: Cancel binding retains token until unbind; only one admitted flight per token, never reuse.
 * Bind удерживает token до unbind; только один flight на token. */
int styxcancelbind(StyxCancel *, StyxMux *);
int styxcancelled(StyxCancel *);

/* EN: Create an owned token for native mount I/O with bounded initial tag waiting.
 * Cancellation and the mux timeout bound admission before any frame can be sent.
 * RU: Создать свой токен для ожидания начального тега в операции маунта.
 * Допуск до отправки ограничен отменой и таймаутом mux. */
StyxCancel *styxcancelcreatewait(void);


/* EN: Read the borrowed token's immutable admission policy.
 * RU: Прочитать неизменяемое правило допуска заимствованного токена. */
int styxcancelwaits(const StyxCancel *cancel);
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


/* EN/RU: Local streaming capability requires live carrier owner with at least two RPC slots.
 * Local streaming capability требует live carrier owner минимум с двумя RPC slots. */
int styxmuxwatchable(StyxMux *mux);
#endif
