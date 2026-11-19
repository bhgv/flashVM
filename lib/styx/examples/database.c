/* Copyright (c) 2026 bhgv
 * EN: Live key/value file as a small database backend: fresh keys/values per query, no startup tree
 * snapshot.
 * RU: Живой файл ключ/значение как малая БД: новые ключи и значения при каждом запросе, без снимка
 * дерева на старте.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#include "catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *database; /* Borrowed argv path, valid until server shutdown. */


/* EN: Borrow nonempty source as the TSV path until server exit; return 1/0 for presence only. Do
 * not open or snapshot it yet. Called once by the server owner.
 * RU: Заимствовать непустой source как TSV path до выхода сервера; вернуть 1/0 только по наличию.
 * Пока не открывать и не снимать snapshot. Вызывает server owner один раз.
 */
int catalog_init(const char *source)
{
    database = source;
    return source && source[0];
}


/* EN: Hash the borrowed NUL-terminated key bytes with unsigned64 FNV-1a; map zero away from the
 * root ID. Return the scalar, retain no pointer. This is stable key identity, not authentication;
 * catalog_query rejects collisions within each loaded set.
 * RU: Хэшировать заимствованные NUL-terminated bytes ключа unsigned64 FNV-1a; заменить ноль,
 * занятый root ID. Вернуть scalar без хранения ссылки. Это identity ключа, не authentication;
 * catalog_query отвергает collisions внутри загруженного набора.
 */
static uint64_t identity(const char *name)
{
    uint64_t result = UINT64_C(14695981039346656037);
    while (*name)
    {
        result = (result ^ (unsigned char)*name++) * UINT64_C(1099511628211);
    }
    return result ? result : 1;
}


/* EN: Reopen the borrowed TSV path on every call. listing=0 looks up id (root stat uses start=0);
 * otherwise list root children after start matching rows. Borrow out/count, cap copies by capacity;
 * return 1 with count (possibly zero), or 0 for open/read/format/duplicate-ID failure. At most 128
 * rows are staged on the stack and validated before publication. Close the FILE on every branch;
 * synchronous server-owner I/O, no persistent record lease.
 * RU: Заново открыть TSV path при каждом вызове. listing=0 ищет id (root stat требует start=0),
 * иначе перечислить детей root после start совпадений. out/count заимствованы, копий не больше
 * capacity; вернуть 1 с count (возможно 0) либо 0 при open/read/format/duplicate-ID error. До
 * публикации проверить до 128 stack rows. Закрывать FILE на каждой ветке; синхронный server-owner
 * I/O, без lease записи.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count)
{
    FILE *file = fopen(database, "r");
    CatalogRow rows[128];
    size_t used = 0;
    char line[2200];
    *count = 0;
    if (!file)
    {
        return 0;
    }
    while (fgets(line, sizeof(line), file))
    {
        char *tab = strchr(line, '\t');
        size_t length = strlen(line);
        if (!tab || used == 128 || length == sizeof(line) - 1 || tab == line ||
            (size_t)(tab - line) >= sizeof(rows[0].name))
        {
            fclose(file);
            return 0;
        }
        *tab++ = 0;
        tab[strcspn(tab, "\r\n")] = 0;
        if (strchr(line, '/') || !strcmp(line, ".") || !strcmp(line, "..") || strlen(tab) >= sizeof(rows[0].value))
        {
            fclose(file);
            return 0;
        }
        memset(&rows[used], 0, sizeof(rows[used]));
        rows[used].id = identity(line);
        rows[used].mode = 0444;
        strcpy(rows[used].name, line);
        strcpy(rows[used].value, tab);
        for (size_t previous = 0; previous < used; previous++)
        {
            if (rows[previous].id == rows[used].id)
            {
                fclose(file);
                return 0;
            }
        }
        used++;
    }
    int valid = !ferror(file);
    fclose(file);
    if (!valid)
    {
        return 0;
    }
    if (!listing && !id && capacity)
    {
        CatalogRow root = {0, 0, CATALOG_DIR | 0555, "/", ""};
        out[0] = root;
        *count = 1;
        return 1;
    }
    size_t seen = 0;
    for (size_t index = 0; index < used; index++)
    {
        if ((listing && !id) || (!listing && rows[index].id == id))
        {
            if (seen++ >= start && *count < capacity)
            {
                out[(*count)++] = rows[index];
            }
        }
    }
    return 1;
}


/* EN: All id/input/size/offset arguments are unused; return -1. This projection never modifies the
 * TSV source or retries a write.
 * RU: Все id/input/size/offset не используются; вернуть -1. Проекция не изменяет TSV source и не
 * повторяет запись.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset)
{
    (void)id;
    (void)input;
    (void)size;
    (void)offset;
    return -1;
}


/* EN: id is a validated logical key ID; return 1 without opening a host FILE or retaining a row.
 * Later reads query current TSV contents, so disappearance can fail and reappearance of the same
 * key has the same identity.
 * RU: id — проверенный logical key ID; вернуть 1 без открытия host FILE или удержания строки.
 * Следующие reads запрашивают текущий TSV: исчезновение может дать ошибку, тот же ключ после
 * возврата имеет прежний ID.
 */
int catalog_pin(uint64_t id)
{
    (void)id;
    return 1;
}


/* EN: id is unused: no per-open host resource was retained. Closing a fid does not remove or
 * rewrite a database key.
 * RU: id не используется: per-open host resource не удерживался. Закрытие fid не удаляет и не
 * перезаписывает ключ БД.
 */
void catalog_unpin(uint64_t id)
{
    (void)id;
}
