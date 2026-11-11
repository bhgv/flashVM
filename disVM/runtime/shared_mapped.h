/* Internal mapped-value transaction; caller supplies a validated live Type. */
#ifndef DVM_SHARED_MAPPED_H
#define DVM_SHARED_MAPPED_H
#include <stddef.h>

struct Array;
struct Prog;
struct Type; /* Borrowed retained native descriptor, defined by interp.h. */


/* [GC:] Copy protected mapped record once, retaining incoming references under
 * cell/range guards and rooting displaced owners before publication. Native
 * preparation may fail before effects. Errors/destructors run only after unlock.
 * Caller independently owns live mapped source/destination and immutable Type;
 * size must be its already validated native extent. Requires current-owner GC
 * admission; other residents may execute concurrently under range guards.
 * RU: source/destination/type живут у вызывающего; допуск к GC не сериализует
 * потоки. Владение ссылками и публикация защищены отдельными диапазонами.
 * Returns normally after retirement; VM errors propagate after native cleanup. */
void dvm_runtime_mapped_copy(const void *source, void *destination, struct Type *type, int size);
/* Move ownership from a disjoint, live ring slot. Clears source pointers
 * in the guarded publication. consumed stays set if retirement later raises. */
void dvm_runtime_mapped_consume(void *source, void *destination, struct Type *type,
    int size, int *consumed);


/* As for mapped_copy, with pre-effect BUSY replay for the decoded MOVMP. */
void dvm_runtime_mapped_move_opcode(const void *source, void *destination, struct Type *type, int size);


/* [GC:] Copy a validated count of elements as one transaction. Borrow mapped
 * spans and immutable type under GC admission. BUSY can replay before changes.
 * RU: Заимствовать живые диапазоны/type при допуске GC; повтор возможен до изменений. */
void dvm_runtime_array_slice_copy(const void *source, void *destination, struct Type *type, size_t count,
    struct Array *source_array, struct Array *destination_array);
extern int dvm_runtime_parallel_slicela;
extern int dvm_runtime_parallel_movmp;
extern int dvm_runtime_parallel_channel_buffer;
extern unsigned long dvm_runtime_parallel_channel_sends; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_channel_raw; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_channel_refills; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_channel_alts; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_rendezvous; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_channel_receives; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_movmp_commits; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_movmp_module_commits; /* Atomic completed-transfer count; no ownership publication. */
extern unsigned long dvm_runtime_parallel_slicela_commits; /* Atomic completed-transfer count; no ownership publication. */
/* [GC:] Transfer live ring spans. A consuming receive clears source pointers.
 * Both directions report commit even when retirement raises. Return zero on replay. */
int dvm_runtime_mapped_buffer_transfer(const void *source, void *destination, struct Type *type,
    int consume, int *committed, int replay_opcode, const struct Prog *refill_sender);
void dvm_runtime_mapped_rendezvous(const void *source, void *destination,
    struct Type *type, int *committed);
#endif
