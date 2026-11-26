/* Copyright (c) 2026 bhgv
 * EN: Exercise capability identity and in-flight rendezvous lease retirement.
 * RU: Проверить идентичность полномочий и завершение аренды при передаче.
 */
#include "styxrpc/named_cell.h"
#include <stdio.h>
#include <string.h>


/* EN: Repeat entropy deliberately, so the test depends on a non-reused serial.
 * RU: Повторять энтропию намеренно, чтобы проверить непереиспользуемый номер. */
static int entropy(void *state, unsigned char *bytes, size_t size)
{
    (void)state;
    memset(bytes, 0xA5, size);
    return 1;
}


/* EN: Submit header and body as separate wire fragments.
 * RU: Передать заголовок и тело двумя сетевыми фрагментами. */
static int request(StyxRpcNamedEndpoint *endpoint, unsigned operation, const void *body, size_t size)
{
    unsigned char header[STYXRPC_NAMED_HEADER] = {(unsigned char)operation, 0, 0, 0, (unsigned char)size};
    return endpoint && styxrpc_named_submit(endpoint, 0, header, sizeof(header)) &&
        (!size || styxrpc_named_submit(endpoint, sizeof(header), body, size));
}


/* EN: Read one expected short reply in separate length and payload reads.
 * RU: Прочитать ожидаемый короткий ответ отдельными чтениями длины и данных. */
static int response(StyxRpcNamedEndpoint *endpoint, void *body, size_t size)
{
    unsigned char header[STYXRPC_NAMED_REPLY];
    size_t used;
    return styxrpc_named_reply(endpoint, 0, header, sizeof(header), &used) &&
        used == sizeof(header) && !header[0] && !header[1] && !header[2] && header[3] == size &&
        (!size || (styxrpc_named_reply(endpoint, sizeof(header), body, size, &used) && used == size));
}


/* EN: Retire the sender before the receiver attaches, then prove final revocation.
 * RU: Закрыть отправителя до получения аренды и подтвердить окончательный отзыв. */
static int check(StyxRpcNamedRegistry *registry)
{
    const char path[] = "/jobs";
    const unsigned char initial[] = {STYXRPC_NAMED_BOOLEAN, 1};
    const unsigned char changed[] = {STYXRPC_NAMED_TEXT, 'b', 0, 'c'};
    const unsigned char invalid[] = {STYXRPC_NAMED_BOOLEAN, 2};
    unsigned char frame[1 + STYXRPC_CELL_TOKEN] = {STYXRPC_NAMED_CELL_REF};
    unsigned char forwarded[sizeof(frame)], value[sizeof(changed)], fresh[STYXRPC_CELL_TOKEN];
    StyxRpcNamedEndpoint *creator = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *writer = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *reader = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *attached = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *invalid_writer = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *replacement = styxrpc_named_endpoint(registry);
    StyxRpcNamedEndpoint *stale = styxrpc_named_endpoint(registry);
    size_t waiting = 1;
    int ok = creator && writer && reader && attached && invalid_writer && replacement && stale &&
        request(creator, STYXRPC_CELL_NEW, initial, sizeof(initial)) &&
        response(creator, frame + 1, STYXRPC_CELL_TOKEN) &&
        request(writer, STYXRPC_NAMED_OPEN, path, sizeof(path) - 1) &&
        request(reader, STYXRPC_NAMED_OPEN, path, sizeof(path) - 1) &&
        request(writer, STYXRPC_NAMED_WRITE, frame, sizeof(frame)) &&
        styxrpc_named_reply(writer, 0, value, sizeof(value), &waiting) && !waiting;
    styxrpc_named_retire(creator);
    ok = ok && request(reader, STYXRPC_NAMED_READ, NULL, 0) &&
        response(writer, NULL, 0) && response(reader, forwarded, sizeof(forwarded)) &&
        !memcmp(forwarded, frame, sizeof(frame));
    styxrpc_named_retire(writer);
    ok = ok && request(attached, STYXRPC_CELL_ATTACH, forwarded + 1, STYXRPC_CELL_TOKEN);
    styxrpc_named_retire(reader);
    ok = ok && request(attached, STYXRPC_CELL_GET, NULL, 0) &&
        response(attached, value, sizeof(initial)) && !memcmp(value, initial, sizeof(initial)) &&
        request(attached, STYXRPC_CELL_SET, changed, sizeof(changed)) &&
        request(invalid_writer, STYXRPC_CELL_ATTACH, forwarded + 1, STYXRPC_CELL_TOKEN) &&
        !request(invalid_writer, STYXRPC_CELL_SET, invalid, sizeof(invalid)) &&
        request(attached, STYXRPC_CELL_GET, NULL, 0) &&
        response(attached, value, sizeof(changed)) && !memcmp(value, changed, sizeof(changed));
    styxrpc_named_retire(invalid_writer);
    styxrpc_named_retire(attached);
    ok = ok && request(replacement, STYXRPC_CELL_NEW, changed, sizeof(changed)) &&
        response(replacement, fresh, sizeof(fresh)) && memcmp(fresh, frame + 1, sizeof(fresh)) &&
        !request(stale, STYXRPC_CELL_ATTACH, frame + 1, STYXRPC_CELL_TOKEN);
    styxrpc_named_retire(replacement);
    styxrpc_named_retire(stale);
    return ok;
}


/* EN: Construct and retire one test registry. Return nonzero on any contract failure.
 * RU: Создать и удалить тестовый реестр. Вернуть ненулевой код при нарушении контракта. */
int main(void)
{
    StyxRpcNamedRegistry *registry = styxrpc_named_registry(NULL, NULL, NULL, NULL);
    int ok = styxrpc_named_cells_enable(registry, entropy, NULL) && check(registry);
    styxrpc_named_destroy(registry);
    if (!ok)
    {
        fputs("named cell transfer or lease lifetime differed\n", stderr);
        return 1;
    }
    return 0;
}
