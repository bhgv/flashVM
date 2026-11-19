/* Copyright (c) 2026 bhgv
 * EN: Fixed one-level or three-level catalogs selected at compile time, sharing the same original
 * Styx callbacks.
 * RU: Фиксированные каталоги в один либо три уровня, выбранные при сборке, с общими callbacks
 * исходной Styx.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#include "catalog.h"
#include <string.h>
#ifdef CATALOG_TREE
static const CatalogRow rows[] = {
    {0, 0, CATALOG_DIR | 0555, "/", ""},
    {10, 0, CATALOG_DIR | 0555, "devices", ""},
    {11, 10, CATALOG_DIR | 0555, "sensor", ""},
    {12, 11, 0444, "temperature", "21.5\n"},
    {20, 0, CATALOG_DIR | 0555, "system", ""},
    {21, 20, 0444, "info", "standalone Styx example\n"}}; /* Immutable fixed hierarchy, no OS device dependencies. */
#else
static const CatalogRow rows[] = {{0, 0, CATALOG_DIR | 0555, "/", ""},
                                  {1, 0, 0444, "status", "ready\n"},
                                  {2, 0, 0444, "temperature", "21.5\n"}}; /* Fixed root children. */
#endif


/* EN: source is unused; return 1 because the immutable compile-time table needs no setup or
 * ownership. Called once before server requests.
 * RU: source не используется; вернуть 1: immutable compile-time table не требует setup/ownership.
 * Вызвать один раз до запросов.
 */
int catalog_init(const char *source)
{
    (void)source;
    return 1;
}


/* EN: listing=0 selects stable id, otherwise its immediate children; start skips matching rows.
 * Copy at most capacity rows into borrowed out, set borrowed count and return 1, including
 * empty/end. Root is not its own listed child. No I/O, allocation or retained pointers; server
 * owner serializes backend calls.
 * RU: listing=0 выбирает stable id, иначе непосредственных детей; start пропускает совпавшие
 * строки. Копировать не больше capacity в out, задать count, вернуть 1, включая отсутствие/конец.
 * Root не входит в собственный listing. Без I/O, allocation и хранения ссылок; вызовы сериализует
 * server owner.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count)
{
    size_t seen = 0;
    *count = 0;
    for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index++)
    {
        if ((!listing && rows[index].id == id) || (listing && rows[index].id && rows[index].parent == id))
        {
            if (seen++ >= start && *count < capacity)
            {
                out[(*count)++] = rows[index];
            }
        }
    }
    return 1;
}


/* EN: id/input/size/offset are intentionally unused: every write returns -1 without touching data.
 * The table is readonly, regardless of the requested name.
 * RU: id/input/size/offset намеренно не используются: любая запись даёт -1 без изменений. Таблица
 * readonly независимо от имени.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset)
{
    (void)id;
    (void)input;
    (void)size;
    (void)offset;
    return -1;
}


/* EN: id was already validated by the server. Return 1 without acquiring a resource because
 * immutable rows live for the entire process. No hidden-name or refcount state exists.
 * RU: id уже проверен сервером. Вернуть 1 без приобретения ресурса: immutable rows живут весь
 * процесс. Состояния скрытых имён/refcount нет.
 */
int catalog_pin(uint64_t id)
{
    (void)id;
    return 1;
}


/* EN: id is unused; release nothing because catalog_pin retained no resource. This balances the
 * server open/close contract without modifying the table.
 * RU: id не используется; ничего не освобождать, catalog_pin не приобрёл ресурс. Балансировать
 * server open/close contract без изменения таблицы.
 */
void catalog_unpin(uint64_t id)
{
    (void)id;
}
