/* Copyright (c) 2026 bhgv
 * EN: Query a paginated HTTP backend through a bounded helper process, without a shell or native
 * HTTP dependency.
 * RU: Запрашивать постраничный HTTP backend ограниченным helper-процессом, без shell и нативной
 * HTTP-зависимости.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#define _POSIX_C_SOURCE 200809L
#include "catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
static const char *endpoint; /* Borrowed argv URL, never interpolated into a shell command. */


/* EN: Borrow nonempty source as the endpoint until server exit; return 1/0 for presence. URL/schema
 * validation happens in the helper per query, not during initialization. One server owner uses this
 * state.
 * RU: Заимствовать непустой source как endpoint до выхода; вернуть 1/0 по наличию. URL/schema
 * проверяет helper при запросе, не init. State использует один server owner.
 */
int catalog_init(const char *source)
{
    endpoint = source;
    return source && source[0];
}


/* EN: Decode borrowed hex into borrowed output of capacity bytes including NUL. A single dash
 * encodes empty text; pairs must parse to nonzero bytes. Return 1 on complete conversion, 0 on
 * length/pair failure; failure output is provisional and must be discarded. Input is the helper-
 * generated canonical hex, not an arbitrary wire field.
 * RU: Декодировать hex в output ёмкостью capacity с NUL. Dash означает пустой текст, пары должны
 * разбираться в ненулевые bytes. Вернуть 1 при полном преобразовании, 0 при length/pair error;
 * partial output отбросить. Input — canonical hex от helper, не произвольное wire field.
 */
static int text_field(const char *hex, char *output, size_t capacity)
{
    if (!strcmp(hex, "-"))
    {
        output[0] = 0;
        return 1;
    }
    size_t size = strlen(hex);
    if (size % 2 || size / 2 >= capacity)
    {
        return 0;
    }
    for (size_t index = 0; index < size; index += 2)
    {
        char pair[3] = {hex[index], hex[index + 1], 0};
        char *end;
        long value = strtol(pair, &end, 16);
        if (*end || value <= 0 || value > 255)
        {
            return 0;
        }
        output[index / 2] = value;
    }
    output[size / 2] = 0;
    return 1;
}


/* EN: listing chooses list/stat of id, start skips immediate children, capacity must be
 * 1..CATALOG_PAGE. Borrow out/count through return; count starts at zero but rows/count are
 * provisional until return 1. Own pipe ends, child and FILE; exec the configured Python helper
 * without shell, drain its output and waitpid even on parse failure. Return 0 on
 * setup/read/parse/nonzero-child failure. Blocks this server owner; helper bounds HTTP to 3s/128KiB
 * and its process to 5s.
 * RU: listing выбирает list/stat id, start пропускает детей, capacity=1..CATALOG_PAGE. out/count
 * заимствованы; count сначала 0, но rows/count provisional до возврата 1. Владеть pipe ends, child
 * и FILE; exec Python helper без shell, дочитать output и вызвать waitpid даже при parse failure.
 * Вернуть 0 при setup/read/parse/nonzero-child. Блокирует server owner; helper ограничивает HTTP
 * 3с/128KiB, процесс — 5с.
 */
int catalog_query(int listing, uint64_t id, size_t start, CatalogRow *out, size_t capacity, size_t *count)
{
    int descriptors[2];
    char number[32], offset[32], limit[32];
    *count = 0;
    if (!capacity || capacity > CATALOG_PAGE || pipe(descriptors))
    {
        return 0;
    }
    snprintf(number, sizeof(number), "%llu", (unsigned long long)id);
    snprintf(offset, sizeof(offset), "%zu", start);
    snprintf(limit, sizeof(limit), "%zu", capacity);
    pid_t child = fork();
    if (!child)
    {
        close(descriptors[0]);
        if (dup2(descriptors[1], STDOUT_FILENO) < 0)
        {
            _exit(126);
        }
        close(descriptors[1]);
        execl(CATALOG_PYTHON,
              CATALOG_PYTHON,
              CATALOG_HELPER,
              endpoint,
              listing ? "list" : "stat",
              number,
              offset,
              limit,
              (char *)NULL);
        _exit(127);
    }
    close(descriptors[1]);
    if (child < 0)
    {
        close(descriptors[0]);
        return 0;
    }
    FILE *stream = fdopen(descriptors[0], "r");
    char line[4600];
    int valid = stream != NULL;
    if (!stream)
    {
        close(descriptors[0]);
    }
    while (stream && fgets(line, sizeof(line), stream))
    {
        unsigned long long object, parent;
        unsigned long mode;
        char name[256], value[4096], trailing;
        if (*count == capacity ||
            sscanf(line, "%llu %llu %lo %255s %4095s %c", &object, &parent, &mode, name, value, &trailing) != 5)
        {
            valid = 0;
            continue;
        }
        CatalogRow *row = &out[(*count)++];
        memset(row, 0, sizeof(*row));
        row->id = object;
        row->parent = parent;
        row->mode = mode;
        valid = valid && text_field(name, row->name, sizeof(row->name)) &&
                text_field(value, row->value, sizeof(row->value));
    }
    if (stream)
    {
        valid = valid && !ferror(stream);
        fclose(stream);
    }
    int status;
    pid_t joined;
    do
    {
        joined = waitpid(child, &status, 0);
    } while (joined < 0 && errno == EINTR);
    return valid && joined == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}


/* EN: id/input/size/offset are unused; return -1 without an HTTP request. This example publishes a
 * readonly projection and never replays mutations.
 * RU: id/input/size/offset не используются; вернуть -1 без HTTP request. Это readonly projection,
 * mutations не повторяются.
 */
long catalog_write(uint64_t id, const char *input, size_t size, uint64_t offset)
{
    (void)id;
    (void)input;
    (void)size;
    (void)offset;
    return -1;
}


/* EN: id is already validated; return 1 but acquire no remote lease or local snapshot. Subsequent
 * read/stat fetches the logical ID again and may fail if the provider removes it.
 * RU: id уже проверен; вернуть 1 без remote lease/local snapshot. Следующий read/stat вновь
 * запрашивает logical ID и может отказать после удаления провайдером.
 */
int catalog_pin(uint64_t id)
{
    (void)id;
    return 1;
}


/* EN: id is unused; catalog_pin acquired no per-open HTTP resource. No remote deletion or request
 * is triggered by close.
 * RU: id не используется; catalog_pin не приобрёл per-open HTTP resource. Close не вызывает
 * удаление или remote request.
 */
void catalog_unpin(uint64_t id)
{
    (void)id;
}
