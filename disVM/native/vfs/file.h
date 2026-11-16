/* Copyright (c) 2026 bhgv
 * EN: Scalar C boundary between Inferno callbacks and owned VFS file capabilities.
 * RU: Скалярная C-граница callbacks Inferno и владеющих VFS-capabilities файла.
 */
#ifndef STYX_FILE_H
#define STYX_FILE_H
#include <stddef.h>
typedef struct StyxBacking StyxBacking; /* Owned prepared file capability. */
typedef struct StyxOpen StyxOpen; /* One exclusive open, never shared between fids. */
typedef struct StyxMeta {
	unsigned long long identity, length; /* Full-width native identity and length. */
	unsigned long mode; /* Display permissions, never an authority grant. */
	int directory; /* Directory capabilities are refused by this first file bridge. */
} StyxMeta;
typedef struct StyxTransfer {
	size_t count; /* Confirmed prefix only. */
	int success, confirmed, uncertain, stop; /* Original native outcome, no automatic retry. */
} StyxTransfer;

/* EN/RU: Consume backing; existing opens independently pin provider lifetime.
 * Поглотить backing; открытые handles независимо удерживают provider. */
void styxbackingfree(StyxBacking *backing);
/* EN/RU: Query live backing metadata; borrowed arguments, 1/0 success/failure.
 * Получить живые metadata backing; аргументы заимствованы, 1/0. */
int styxbackinginfo(StyxBacking *backing, StyxMeta *output);
/* EN/RU: Open provider r/wo/rw (0/1/2), without requesting create/truncate; owned result or NULL.
 * Открыть provider r/wo/rw (0/1/2) без create/truncate; свой результат либо NULL. */
StyxOpen *styxfileopen(StyxBacking *backing, int mode);
/* EN/RU: Consume one open after all serialized borrowers retire; NULL allowed.
 * Поглотить open после завершения заёмщиков; NULL допустим. */
void styxfileclose(void *opened);
/* EN/RU: Query metadata through a retained open handle, without reopening a path.
 * Получить metadata через удержанный handle, не открывая путь заново. */
int styxfileinfo(StyxOpen *opened, StyxMeta *output);
/* EN/RU: One serialized native chunk at full-width offset; never replay effects.
 * Один последовательный native chunk по полному offset; без повтора effects. */
void styxfileread(StyxOpen *opened, unsigned long long offset, void *bytes, size_t size, StyxTransfer *output);
/* EN/RU: Same contract for immutable write bytes; zero progress is not EOF.
 * Тот же контракт для записи; отсутствие прогресса не является EOF. */
void styxfilewrite(StyxOpen *opened, unsigned long long offset, const void *bytes, size_t size, StyxTransfer *output);
#endif
