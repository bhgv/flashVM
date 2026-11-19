/* Copyright (c) 2026 bhgv
 * EN: Mutable window/control namespace for a single native Styx server owner.
 * Names can disappear while opened IDs remain pinned; slots and Qids have
 * different lifetimes. No GUI toolkit, VM or global pause is involved.
 * RU: Изменяемое пространство окон/контролов одного native Styx server owner.
 * Имена исчезают, но открытые ID удерживаются; время жизни места и Qid различно.
 * GUI toolkit, VM и глобальная остановка не требуются.
 * standalone-reviewed: authored registry contracts and control grammar.
 */
#include "catalog.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* EN/RU: One shared lifetime for a directory and its two children.
 * Общее время жизни каталога и двух его дочерних файлов.
 */
typedef struct Window
{
    uint64_t id;         /* Stable never-reused base Qid. */
    unsigned references; /* Actual original-server opened fids retaining this object. */
    int visible;         /* Included in fresh root walks/listings only while set. */
} Window;

static Window windows[16];     /* Bounded live/held object slots, reusable only after final hidden close. */
static uint64_t next_id = 100; /* Never reused; each window reserves three consecutive stable IDs. */


/* EN: Initialize once before serving requests; source is unused and never retained.
 * Return 1 after publishing the first window. Static state belongs to this server
 * process; this is neither a reset API nor safe during concurrent requests.
 * RU: Инициализировать один раз до обслуживания; source не используется и не хранится.
 * Вернуть 1 после публикации первого окна. Static state принадлежит server process;
 * это не API сброса, конкурентные вызовы недопустимы.
 */
int catalog_init(const char *source)
{
    (void)source;
    windows[0].id = next_id;
    windows[0].visible = 1;
    next_id += 3; /* Reserve window, title and button Qids as one never-reused group. */
    return 1;
}


/* EN: Copy candidate row after skipping start matches; seen/count are borrowed
 * counters, out has capacity rows. Increment seen even when the page is full;
 * increment count only for copied rows. No pointers escape, no allocation/wait.
 * RU: Скопировать row после пропуска start совпадений; seen/count — заимствованные
 * счётчики, out вмещает capacity строк. seen растёт и для полной страницы,
 * count — только при копировании. Без сохранения указателей, allocation/wait.
 */
static void emit(CatalogRow row, size_t start, size_t *seen, CatalogRow *out, size_t capacity, size_t *count)
{
    if ((*seen)++ >= start && *count < capacity)
    {
        out[(*count)++] = row;
    }
}


/* EN: listing=0 looks up stable id; nonzero lists its immediate children.
 * start counts matching rows, not encoded 9P bytes; out/count are caller-owned,
 * capacity bounds all copies. Return 1 with count=0 for absent objects/end.
 * Hidden pinned IDs still resolve for held fids, but fresh root listings omit
 * them. Walk/open policy belongs to server.c. The server owner serializes calls.
 * RU: listing=0 ищет stable id; ненулевой — его непосредственных детей.
 * start считает строки, не байты 9P; out/count принадлежат caller, capacity
 * ограничивает копии. Вернуть 1 и count=0 при отсутствии/конце. Скрытые pinned ID
 * доступны удержанным fid, но не новым root listings. Walk/open policy — server.c.
 * Все вызовы сериализованы владельцем сервера.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count)
{
    CatalogRow row = {0, 0, CATALOG_DIR | 0555, "/", ""};
    size_t seen = 0;
    *count = 0;
    if (!listing && !id)
    {
        emit(row, start, &seen, out, capacity, count);
    }
    row.id = 1; /* ctl is a permanent write-only sibling of all published windows. */
    row.mode = 0222;
    strcpy(row.name, "ctl");
    if ((listing && !id) || (!listing && id == 1))
    {
        emit(row, start, &seen, out, capacity, count);
    }
    for (size_t index = 0; index < 16; index++)
    {
        if (!windows[index].id)
        {
            continue;
        }
        /* Parts 0/1/2 are directory/title/button; metadata is copied only on demand. */
        for (unsigned part = 0; part < 3; part++)
        {
            memset(&row, 0, sizeof(row));
            row.id = windows[index].id + part;
            row.parent = part ? windows[index].id : 0;
            row.mode = part ? 0444 : CATALOG_DIR | 0555;
            if (!part)
            {
                snprintf(row.name, sizeof(row.name), "window-%llu", (unsigned long long)row.id);
            }
            else
            {
                strcpy(row.name, part == 1 ? "title" : "button");
                strcpy(row.value, part == 1 ? "Live window" : "enabled");
            }
            /* Direct ID queries serve held fids; root enumeration applies visibility. */
            if ((!listing && row.id == id) || (listing && row.parent == id && (id || windows[index].visible)))
            {
                emit(row, start, &seen, out, capacity, count);
            }
        }
    }
    return 1;
}


/* EN: Parse nonempty decimal digits from borrowed text into borrowed output.
 * Return 1 only for a positive uint64; reject signs, suffixes and overflow before
 * multiplication. Publish output only on success; no locale/errno dependency.
 * RU: Разобрать непустые decimal digits text в output. Вернуть 1 только для
 * положительного uint64; отвергнуть знаки, суффиксы и overflow до умножения.
 * output меняется только при успехе; locale/errno не используются.
 */
static int remove_id(const char *text, uint64_t *output)
{
    uint64_t value = 0; /* Stable ID, not an array index or host pointer. */
    if (!*text)
    {
        return 0;
    }
    for (; *text; text++)
    {
        if (*text < '0' || *text > '9')
        {
            return 0;
        }
        unsigned digit = (unsigned)(*text - '0');
        if (value > (UINT64_MAX - digit) / 10)
        {
            return 0;
        }
        value = value * 10 + digit;
    }
    if (!value)
    {
        return 0;
    }
    *output = value;
    return 1;
}


/* EN: Apply exactly one ctl command at offset zero. id must be 1, input covers
 * size bytes (1..79); optional one LF or CRLF is part of the accepted byte count.
 * Reject embedded NUL/newlines, extra commands, signs, spaces around the ID and
 * decimal overflow. Return size only after mutation, otherwise -1 without mutation.
 * No input is retained; one server owner serializes query/write/pin/unpin.
 * RU: Выполнить ровно одну ctl-команду при offset=0 и id=1; input содержит size
 * байтов (1..79). Один завершающий LF/CRLF входит в подтверждённую длину.
 * Отказать при NUL/внутреннем newline, нескольких командах, знаке, пробелах вокруг
 * ID и decimal overflow. Вернуть size после изменения, иначе -1 без изменений.
 * input не сохраняется; query/write/pin/unpin выполняет один server owner.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset)
{
    char command[80];
    if (id != 1 || offset || !size || size >= sizeof(command) || memchr(input, 0, size))
    {
        return -1;
    }
    memcpy(command, input, size);
    command[size] = 0;
    size_t length = size; /* Work on the bounded local copy, never modify caller memory. */
    if (length && command[length - 1] == '\n')
    {
        command[--length] = 0;
        if (length && command[length - 1] == '\r')
        {
            command[--length] = 0;
        }
    }
    if (memchr(command, '\n', length) || memchr(command, '\r', length))
    {
        return -1; /* A second line must never be silently acknowledged and discarded. */
    }
    if (!strcmp(command, "add") && next_id <= UINT64_MAX - 3)
    {
        for (size_t index = 0; index < 16; index++)
        {
            if (!windows[index].id)
            {
                windows[index].id = next_id;
                windows[index].visible = 1;
                next_id += 3;
                return size;
            }
        }
    }
    if (!strncmp(command, "remove ", 7))
    {
        uint64_t removed;
        if (remove_id(command + 7, &removed))
        {
            for (size_t index = 0; index < 16; index++)
            {
                if (windows[index].id == removed && windows[index].visible)
                {
                    windows[index].visible = 0; /* Detach the name before considering storage reclamation. */
                    if (!windows[index].references)
                    {
                        windows[index].id = 0;
                    }
                    return size;
                }
            }
        }
    }
    return -1;
}


/* EN: Retain one already validated opened id; root/ctl need no registry slot.
 * Return 1 on success, 0 for absent IDs or reference exhaustion. All three IDs
 * of a window share one refcount; hidden objects are not republished by pin.
 * The native server pairs each successful pin with exactly one unpin.
 * RU: Удержать проверенный opened id; root/ctl не требуют места реестра.
 * Вернуть 1 при успехе, 0 при отсутствии/переполнении. Три ID окна разделяют
 * счётчик; pin не публикует скрытое имя. Native server вызывает ровно один unpin
 * для каждого успешного pin. Только поток-владелец сервера.
 */
int catalog_pin(uint64_t id)
{
    if (id < 2)
    {
        return 1;
    }
    for (size_t index = 0; index < 16; index++)
    {
        Window *window = &windows[index];
        if (window->id && id >= window->id && id - window->id < 3 && window->references != UINT_MAX)
        {
            window->references++;
            return 1;
        }
    }
    return 0;
}


/* EN: Release one successful pin of id. The last close of a hidden window frees
 * its slot; its Qids are never recycled. Unknown/unpinned IDs are ignored.
 * No allocation or I/O; only the server owner may change this shared registry.
 * RU: Освободить успешный pin id. Последний close скрытого окна освобождает место,
 * но Qid не переиспользуются. Неизвестные/unpinned ID игнорируются.
 * Без allocation/I/O; менять общий реестр может только server owner.
 */
void catalog_unpin(uint64_t id)
{
    for (size_t index = 0; index < 16; index++)
    {
        Window *window = &windows[index];
        if (window->id && id >= window->id && id - window->id < 3 && window->references)
        {
            window->references--;
            if (!window->visible && !window->references)
            {
                window->id = 0;
            }
            return;
        }
    }
}
