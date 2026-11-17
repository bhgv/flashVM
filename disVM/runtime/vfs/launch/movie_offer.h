/* Copyright (c) 2026 bhgv
 * EN: One trusted job/connection's native movie admission state; names never grant authority.
 * RU: Native movie admission одного trusted job/connection; имена не дают authority.
 */
#ifndef DVM_MOVIE_OFFER_H
#define DVM_MOVIE_OFFER_H
#include "../provider.h"
#define DVM_MOVIE_GENERATION_MAX UINT64_C(0x7fffffffffffffff) /* Reserve upper half for virtual-directory identities. */
#define DVM_MOVIE_NAME 64U /* Bounded logical Twalk path, not a network address or credential. */
typedef struct DVM_MovieOffer DVM_MovieOffer; /* Owned native per-job source; no guest references. */


/* EN/RU: Copied latest state; generation never wraps, ready/closed are0/1.
 * Copied latest state; generation без wrap, ready/closed —0/1. */
typedef struct DVM_MovieOfferInfo
{
    uint64_t generation; /* Monotonic admission generation, initially0 with no owner. */
    int ready; /* Exactly one current granted service is available. */
    int closed; /* Sticky retirement; a new connection/job needs a new source. */
} DVM_MovieOfferInfo;


/* EN/RU: Create owned empty per-job admission source/NULL; native allocation only.
 * Создать owned empty source одного job/connection; NULL при native allocation failure. */
DVM_MovieOffer *dvm_movie_offer_create(void);


/* EN/RU: Retain borrowed source;1/0 overflow, NULL succeeds; no callbacks/I/O.
 * Удержать borrowed source;1/0, NULL допустим, без callbacks/I/O. */
int dvm_movie_offer_retain(DVM_MovieOffer *source);


/* EN/RU: Consume one owner; last release requires retired callers/subscriptions, NULL harmless.
 * Поглотить owner; последний release требует retired callers/subscriptions, NULL безопасен. */
void dvm_movie_offer_release(DVM_MovieOffer *source);


/* EN/RU: Bind first owner with expected0, or replace its exact current generation.
 * Retain borrowed root on success; return new generation/0. No open/read/write or VM entry; displaced roots may run native destructors.
 * Native destructors and journal notices run after locks; first ownership cannot be reused after retirement.
 * Bind первого owner с expected0 либо заменить exact generation; новая generation/0,
 * root удерживается при успехе; native notices вне locks, без open/read/write/VM entry, native destructors вне locks. */
uint64_t dvm_movie_offer_bind(DVM_MovieOffer *source, uint64_t expected, DVM_Provider *root);


/* EN/RU: Reserve initial root without advertising it;1/0, no observer callback.
 * Commit after fallible startup checks; abort retires only an unpublished reservation.
 * Зарезервировать root без объявления;1/0. Commit после fallible startup checks. */
int dvm_movie_offer_stage(DVM_MovieOffer *source, DVM_Provider *root);


/* EN/RU: Publish reserved root; generation1/0, observers notified outside locks.
 * Объявить reserved root; generation1/0, observers вне locks. */
uint64_t dvm_movie_offer_commit(DVM_MovieOffer *source);


/* EN/RU: Retire only an unpublished reservation;1/0, notify CLOSED without READY.
 * Завершить только unpublished reservation;1/0, CLOSED без READY. */
int dvm_movie_offer_abort(DVM_MovieOffer *source);


/* EN/RU: Retire exact owner generation, or any generation for expected0;1 changed/0 stale.
 * Future lookup refuses; already opened objects retain ordinary explicit-capability lifetime.
 * Завершить exact owner либо любой при expected0;1/0, новые lookup запрещены. */
int dvm_movie_offer_close(DVM_MovieOffer *source, uint64_t expected);


/* EN/RU: Copy coherent state under mutex;1/0, output unchanged on invalid input.
 * Копировать coherent state под mutex;1/0, invalid не меняет output. */
int dvm_movie_offer_info(DVM_MovieOffer *source, DVM_MovieOfferInfo *out);


/* EN/RU: Acquire owned current root only for exact advertised name in this trusted source.
 * No global registry, host path, network dial, username authentication or guessed-name authority.
 * Свой current root только по exact name внутри trusted source, без global registry/dial. */
DVM_Provider *dvm_movie_offer_acquire(DVM_MovieOffer *source, const char *name);


/* EN/RU: Format canonical logical name into caller output;1/0, generation must be nonzero.
 * Сформировать canonical logical name;1/0, generation должна быть ненулевой. */
int dvm_movie_offer_name(uint64_t generation, char out[DVM_MOVIE_NAME]);


/* EN/RU: Return owned journal pin/NULL for autonomous native I/O readiness; no guest polling.
 * Snapshot info after a notice; coalescing may replace intermediate generations.
 * Свой journal pin/NULL для native I/O readiness; после notice перечитать info, без guest polling. */
DVM_Changes *dvm_movie_offer_observe(DVM_MovieOffer *source);
#endif
