/* Copyright (c) 2026 bhgv
 * EN: Exact native Bigtable owner classification for GC, without exporting payloads.
 * RU: Точные типы нативных владельцев Bigtable для GC без экспорта payload.
 */
#ifndef DVM_BIGTABLE_OWNER_H
#define DVM_BIGTABLE_OWNER_H


/* [GC:] EN: Runtime-lifetime owner kinds, never guest-provided type tags.
 * RU: Виды владельцев времени жизни runtime, не гостевые теги типов. */
enum
{
    DVM_BIGTABLE_NATIVE_TABLE = 1, /* Scalar table with a quiescent native guard at last release. */
    DVM_BIGTABLE_NATIVE_KEYS = 2 /* Independent immutable native key snapshot. */
};


/* [GC:] EN: Classify borrowed heap by startup-initialized exact type identity.
 * Return the native kind or zero. No payload access, allocation or mutation.
 * RU: Проверить heap по точному типу, установленному при старте.
 * Вернуть вид либо ноль. Без доступа к payload, выделения и изменения. */
int dvm_bigtable_native_kind(const Heap *heap);


/* [GC:] EN: Verify native lease/FIFO metadata is empty before last-owner cleanup.
 * heap is already claimed by GC with no live borrowers. Take only the scheduler
 * metadata mutex; no native payload is freed or entered here. Return 1 if quiet.
 * RU: Проверить отсутствие lease/FIFO перед очисткой последнего владельца.
 * heap уже захвачен GC без живых заёмщиков. Взять только мьютекс метаданных;
 * не входить в payload и не освобождать его. Вернуть 1 при покое. */
int dvm_bigtable_native_quiet(const Heap *heap);
struct TableStorage;
struct FvmTableRequest;
struct TableStorage *dvm_bigtable_storage_create(int *status);
struct TableStorage *dvm_bigtable_storage_retain(void *value);
void dvm_bigtable_storage_release(struct TableStorage *storage);
int dvm_bigtable_storage_request(struct TableStorage *storage, struct FvmTableRequest *request);

struct Fht;

typedef struct DVM_BigtableEdit
{
    void *object; /* Borrowed rooted Bigtable handle. */
    struct Fht *(*prepare)(const struct Fht *, void *); /* Stage an owned replacement or return NULL. */
    void *state; /* Borrowed input for prepare, native-only. */
} DVM_BigtableEdit;

/* EN: Observe an exact native handle under its table guard. The callback must
 * neither throw nor retain payload. Return zero if busy, invalid or refused.
 * RU: Прочитать точный дескриптор под guard таблицы. Callback не выбрасывает
 * исключений и не удерживает payload. Ноль при занятости, ошибке или отказе. */
int dvm_bigtable_snapshot(void *object, int (*visit)(const struct Fht *, void *), void *state);

/* EN: Make one rooted caller-owned handle. Consume payload only on success.
 * A VM allocation can raise before taking ownership.
 * RU: Создать один дескриптор с корнем вызывающей стороны. Забрать payload
 * только при успехе. Аллокация VM может выбросить исключение до передачи. */
void *dvm_bigtable_from_payload(struct Fht *payload);

/* EN: Prepare all replacements under their native guards before committing.
 * Borrow edits and rooted handles. Any contention or failed prepare applies none.
 * Callbacks only use native storage and must not throw or reenter the table runtime.
 * RU: Подготовить все замены под guard до применения. Заимствовать edits и
 * укоренённые дескрипторы. Занятость или ошибка отменяет пакет целиком.
 * Callback использует нативное хранилище и не входит обратно в runtime таблиц. */
int dvm_bigtable_apply_edits(const DVM_BigtableEdit *edits, size_t count);
#endif
