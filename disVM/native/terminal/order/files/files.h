/* Copyright (c) 2026 bhgv
 * EN: Explicit per-client ordinary VFS record files over one owned ordered broker.
 * RU: Явные обычные VFS record-файлы клиентов одного owned ordered-брокера.
 */
#ifndef STYX_ORDERED_FILES_H
#define STYX_ORDERED_FILES_H
#include "broker/broker.h"
#include "provider.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct StyxOrderedFiles StyxOrderedFiles; /* Native lifecycle owner; retire exported factories/handles before free. */


/* EN/RU: Consume peer on every outcome, return owned file-set/NULL; no guest/display or additional wire profile.
 * Поглотить peer при любом исходе, вернуть свой file-set/NULL; без guest/display и нового wire-профиля.
 */
StyxOrderedFiles *styxorderedfiles(StyxOrdered *peer);


/* EN/RU: Borrow set, return owned single-file provider for an already granted tag, or NULL.
 * access is nonzero READ/WRITE subset; metadata uses0444/0222. No root/service or other-tag access.
 * Reads return original inner S-record bytes, at most one record per call; short buffers retain the suffix per open.
 * Each nonempty write must contain exactly one complete original S-record with the granted tag.
 * Current native cancellation applies only to the operation; all waits are native, no guest servicing.
 * Заимствовать set, вернуть свой provider одного файла уже выданного tag либо NULL.
 * access — непустое подмножество READ/WRITE, mode0444/0222. Нет доступа к root/service либо чужим tags.
 * Read выдаёт исходную внутреннюю S-запись, не более одной за вызов; suffix сохраняется в своём open.
 * Каждый непустой write содержит ровно одну полную S-запись своего tag. Отмена относится лишь к операции.
 */
DVM_Provider *styxorderedfile(StyxOrderedFiles *files, unsigned long tag, unsigned access);


/* EN/RU: Explicit native owner stop, no join; wake all exported files, ready records remain drainable.
 * Явный owner-stop без join; разбудить все файлы, готовые записи остаются для дочитывания.
 */
void styxorderedfilesstop(StyxOrderedFiles *files);


/* EN/RU: Consume exclusive owner only after all factories/handles retire:1 success,0 leaves live owner unchanged.
 * NULL succeeds. Joins broker workers on this lifecycle caller; never call from provider destructor/worker.
 * Caller retires concurrent set operations before free. Provider destruction itself never joins or performs I/O.
 * Поглотить exclusive owner после всех factories/handles:1 успех,0 сохраняет owner. NULL безопасен.
 * Join broker workers выполняется lifecycle-caller; не вызывать из provider destructor/worker.
 * Перед free завершить concurrent set operations; уничтожение provider не делает join/I/O.
 */
int styxorderedfilesfree(StyxOrderedFiles *files);
#ifdef __cplusplus
}
#endif
#endif
