/* Copyright (c) 2026 bhgv
 *
 * EN: Native field guards and reusable stripe plans.
 * RU: Нативные блокировки полей и переиспользуемые карты захвата.
 */
/* Ordered native byte-span exclusion sharing the existing pointer-cell domain. */
#ifndef DVM_RANGE_ACCESS_H
#define DVM_RANGE_ACCESS_H
#include "disvm/cell_access.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define DVM_RANGE_GUARDS 256 /* Stripe capacity, never an object or thread quota. */


/* Zeroed stable noncopyable owner-local storage; never modify live metadata. */
typedef struct DVM_RangeAccess
{
    DVM_CellDomain *domain; /* Borrowed through end/partial rollback. */
    DVM_NativeContext *context; /* Exact bound owner. */
    DVM_ObjectGuard *guards[DVM_RANGE_GUARDS]; /* Sorted unique selected stripes. */
    unsigned int count; /* Selected stripe count, including zero for empty spans. */
    unsigned int held; /* Acquired prefix, implementation state only. */
} DVM_RangeAccess;


/* Claim complete arbitrary-sized source/destination byte spans, without native
 * allocation or object-owner waits. Same domain protects overlapping pointer
 * and byte access. Empty spans ignore their address and select no stripe.
 * Large spans select all stripes; collisions serialize unrelated accesses.
 * Caller owns mapped ranges/domain lifetime and supplies zero access storage.
 * Wrong geometry/role/BUSY rolls back guards and borrow; invalid nonzero access
 * refuses unchanged. Metadata locks may wait. No VM/managed ownership callback,
 * root acquisition, mapping proof or permission for unguarded competitors. */
DVM_CellResult dvm_range_access_begin(DVM_CellDomain *domain,
    const void *source, size_t source_size, void *destination, size_t destination_size,
    DVM_RangeAccess *access);


/* Claim three arbitrary-sized spans in one ordered union. first/second are
 * borrowed inputs, output is borrowed writable storage; zero sizes ignore their
 * addresses. Same role/lifetime/rollback contract as two-span begin, same domain
 * and unchanged access layout. No allocation, payload read or callback. */
DVM_CellResult dvm_range_access_three_begin(DVM_CellDomain *domain,
    const void *first, size_t first_size, const void *second, size_t second_size,
    void *output, size_t output_size, DVM_RangeAccess *access);


/* Domain-independent stripe selection, computed from stable field addresses. */
typedef struct DVM_RangePlan
{
    unsigned char bits[DVM_RANGE_GUARDS / 8]; /* One bit per selected stripe. */
} DVM_RangePlan;


/* EN: Build plan for count borrowed cells. Return zero without changing plan on refusal.
 * RU: Построить plan для count полей cells. При отказе вернуть ноль и сохранить plan. */
int dvm_range_plan_cells(DVM_RangePlan *plan, void *const *cells, size_t count);

/* EN: Try plan in domain, returning the acquired guards in access on success.
 * RU: Попробовать захват plan в domain. При успехе сохранить блокировки в access. */
DVM_CellResult dvm_range_plan_begin(DVM_CellDomain *domain,
    const DVM_RangePlan *plan, DVM_RangeAccess *access);


/* Native observer state. Initialize to zero and keep domain alive until end. */
typedef struct DVM_RangeWatch
{
    DVM_CellDomain *domain; /* Borrowed while observation is active. */
    DVM_NativeContext *context; /* The observer's bound context. */
    DVM_RangePlan sampled; /* Stripes observed at least once. */
    unsigned long revision[DVM_RANGE_GUARDS]; /* First revision per sampled stripe. */
} DVM_RangeWatch;


/* EN: Start exclusive observation in domain with empty watch.
 * RU: Начать наблюдение в domain через пустой watch. */
int dvm_range_watch_begin(DVM_CellDomain *domain, DVM_RangeWatch *watch);

/* EN: Record or recheck the revisions of held access in watch.
 * RU: Запомнить или сверить версии захваченных access в watch. */
int dvm_range_watch_sample(DVM_RangeWatch *watch, const DVM_RangeAccess *access);

/* EN: Check that every stripe in held access was sampled and remains unchanged.
 * RU: Проверить, что все полосы access наблюдались в watch и сохранили версии. */
int dvm_range_watch_unchanged(const DVM_RangeWatch *watch, const DVM_RangeAccess *access);

/* EN: End watch on its original context. RU: Завершить watch в исходном контексте. */
int dvm_range_watch_end(DVM_RangeWatch *watch);


/* Claim the ordered union of count borrowed pointer-sized cells in domain.
 * Access uses the same ownership, lifetime and failure contract as begin.
 * Selection reads cell addresses only, before any guard is acquired. */
DVM_CellResult dvm_range_access_cells_begin(DVM_CellDomain *domain,
    void *const *cells, size_t count, DVM_RangeAccess *access);


/* Add an independently live span to a live exact-owner access, without releasing
 * original claims or its one domain borrow. Empty size ignores the address.
 * New guards use bounded try-only acquisition, even below existing keys. Success
 * keeps the metadata sorted; end releases reverse canonical order. BUSY/refusal
 * rolls back additions only and leaves the original access unchanged/live.
 * No allocation, VM/owner callback, managed retain/root or provenance proof.
 * Caller must end before retry/error/world transition/wait. Native metadata
 * mutexes keep their existing contract, not a wall-clock guarantee. */
DVM_CellResult dvm_range_access_extend(DVM_RangeAccess *access,
    const void *span, size_t size);


/* End exact current-owner access, including a successfully acquired empty set.
 * Return0 unchanged for wrong owner/depth/state. Caller retains domain/storage;
 * no callback, allocation, payload read or VM transition. Do not hold access
 * across waits, error/VM callbacks or arbitrary OS/native unwinding. */
int dvm_range_access_end(DVM_RangeAccess *access);
#ifdef __cplusplus
}
#endif
#endif
