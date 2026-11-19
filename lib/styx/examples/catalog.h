/* Copyright (c) 2026 bhgv
 * EN: Private backend contract shared by the five catalog examples. Rows are
 * copied metadata, not native wire structs or preconstructed remote trees.
 * The synchronous server owner serializes every call; none of these functions
 * is a thread-safe public library API. Each backend is linked separately.
 * RU: Private backend contract пяти catalog examples. Строки — копии metadata,
 * не native wire structs и не готовые remote trees. Все вызовы сериализует
 * synchronous server owner; это не thread-safe public API библиотеки.
 * Каждый backend линкуется отдельно.
 * standalone-reviewed: ownership, paging and open/close contracts are explicit.
 */
#ifndef EXAMPLE_CATALOG_H
#define EXAMPLE_CATALOG_H
#include <stdint.h>
#include <stddef.h>

#define CATALOG_PAGE 16U         /* Immediate rows per query / Строк на запрос. */
#define CATALOG_DIR 0x80000000UL /* Standard 9P directory bit / Бит каталога 9P. */


/* EN/RU: Caller-owned copy; string storage travels with the row, not its source.
 * Собственная копия caller: строки живут в row, не ссылаются на source.
 */
typedef struct CatalogRow
{
    uint64_t id;         /* Stable logical ID; root=0 / Logical ID, root=0. */
    uint64_t parent;     /* Parent ID; root's parent=0 / ID родителя, root=0. */
    unsigned long mode; /* Directory/rwx bits / Directory/rwx bits. */
    char name[128];      /* NUL-terminated component; root="/" / Имя, root="/". */
    char value[2048];    /* Bounded example text / Ограниченный текст примера. */
} CatalogRow;


/* EN: Initialize once before requests; return 1/0 for backend setup success.
 * source is optional only for fixed/live backends. Database/HTTP borrow its
 * nonempty argv string until server exit; caller retains ownership.
 * RU: Инициализировать один раз до запросов; вернуть 1/0 по успеху setup.
 * source необязателен только для fixed/live. Database/HTTP заимствуют непустую
 * argv string до выхода сервера; владение остаётся у caller.
 */
int catalog_init(const char *source);


/* EN: listing=0 requests stat of id (start=0, capacity>=1); nonzero requests
 * immediate children after start matching rows. start counts rows, not 9P bytes.
 * output holds capacity rows; count receives the copied extent on success=1.
 * Successful count=0 means absent/end. Return 0 on backend error; output/count
 * may be provisional then and must be discarded. No pointer is retained.
 * IDs cannot alias distinct live objects. Backend-specific leases differ;
 * there is no recursive tree fetch or universal snapshot guarantee.
 * RU: listing=0 запрашивает stat id (start=0, capacity>=1); иначе — детей после
 * start совпадений. start считает строки, не байты 9P. output вмещает capacity,
 * count получает число копий при успехе=1. count=0 — отсутствие/конец.
 * Вернуть 0 при backend error; output/count тогда provisional и отбрасываются.
 * Указатели не сохраняются. ID не должны смешивать разные живые объекты.
 * Leases зависят от backend; recursive tree fetch и общего snapshot нет.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *output, size_t capacity, size_t *count);


/* EN: Apply once to logical id using borrowed input of size bytes at offset.
 * Return accepted bytes in 0..size, or -1 for failure; never imply automatic
 * retry. Fixed/database/HTTP refuse writes; live implements one ctl command.
 * The caller must interpret short/zero acknowledgement as returned.
 * RU: Применить один раз к id, заимствуя input длиной size по offset.
 * Вернуть accepted bytes 0..size либо -1; автоматического retry нет.
 * Fixed/database/HTTP отказывают в записи; live исполняет одну ctl-команду.
 * Caller обязан сохранить смысл short/zero acknowledgement.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset);


/* EN: Retain already validated id for one successful open; return 1/0.
 * Success requires exactly one unpin on close/disconnect. Only live acquires
 * a slot reference; fixed/database/HTTP do not acquire a remote record lease.
 * Pin does not authorize access or republish a hidden name.
 * RU: Удержать проверенный id для одного successful open; вернуть 1/0.
 * Успех требует ровно одного unpin при close/disconnect. Только live удерживает
 * slot reference; fixed/database/HTTP не приобретают lease удалённой записи.
 * Pin не даёт прав и не публикует скрытое имя.
 */
int catalog_pin(uint64_t id);


/* EN: Consume one successful pin of the same id, independent of its current
 * name. No result; backend-specific final release may reclaim hidden storage.
 * RU: Поглотить один successful pin того же id независимо от текущего имени.
 * Возврата нет; backend-specific final release может удалить hidden storage.
 */
void catalog_unpin(uint64_t id);
#endif
