/* Copyright (c) 2026 bhgv
 * EN: Check token lifetime, fid isolation and rollback for C module channels.
 * RU: Проверка срока токенов, изоляции fid и отката выдач каналов модуля C.
 */
#include "styxrpc/module_channels.h"
#include <stdio.h>
#include <string.h>

typedef struct Counter
{
    unsigned writes; /* Calls to the authenticated resource. */
    unsigned closes; /* Exact successful opens retired. */
    unsigned char value; /* Byte stored through the lease. */
} Counter;


/* EN: Emit deterministic bytes for tests and optionally simulate entropy failure.
 * RU: Выдать предсказуемые байты теста и при необходимости вернуть ошибку генератора. */
static int entropy(void *context, void *bytes, size_t length)
{
    const int *enabled = context;
    memset(bytes, 0x42, length);
    return *enabled;
}


/* EN: Return the stored byte only at the reply origin.
 * RU: Вернуть сохранённый байт только в начале ответа. */
static int read_byte(void *state, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const Counter *counter = state;
    if (offset || capacity < 1)
    {
        return 0;
    }
    *(unsigned char *)bytes = counter->value;
    *size = 1;
    return 1;
}


/* EN: Require that the module bus translated the first data write to offset zero.
 * RU: Проверить, что шина модуля перенесла первую запись данных к смещению ноль. */
static int write_byte(void *state, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Counter *counter = state;
    if (offset || count != 1)
    {
        return 0;
    }
    counter->value = *(const unsigned char *)bytes;
    counter->writes++;
    *size = 1;
    return 1;
}


/* EN: Count each successfully bound handle released by clunk or revocation.
 * RU: Подсчитать открытые дескрипторы, освобождённые clunk или отзывом. */
static void close_byte(void *state)
{
    ((Counter *)state)->closes++;
}


/* EN: Authenticate one already opened bus fid using a complete token.
 * RU: Проверить открытый fid шины по полному токену. */
static int authorize(const StyxRpcResource *bus, void *cookie, const unsigned char *token)
{
    size_t extent;
    return bus->write(cookie, 0, token, STYXRPC_CHANNEL_TOKEN_SIZE, &extent) &&
        extent == STYXRPC_CHANNEL_TOKEN_SIZE;
}


/* EN: Exercise concurrent leases, reuse and a sticky failed authentication attempt.
 * RU: Проверить одновременные выдачи, повторное использование и отказ на одном fid. */
static int exercise(void)
{
    int enabled = 0;
    Counter a = {0}, b = {0};
    StyxRpcModuleChannels channels;
    StyxRpcModuleChannel slots[2];
    const StyxRpcResource left = {&a, NULL, read_byte, write_byte, close_byte};
    const StyxRpcResource right = {&b, NULL, read_byte, write_byte, close_byte};
    unsigned char first[STYXRPC_CHANNEL_TOKEN_SIZE] = {0};
    unsigned char second[STYXRPC_CHANNEL_TOKEN_SIZE], fresh[STYXRPC_CHANNEL_TOKEN_SIZE];
    const unsigned char zero[STYXRPC_CHANNEL_TOKEN_SIZE] = {0};
    if (!styxrpc_module_channels_init(&channels, slots, 2, entropy, &enabled) ||
        styxrpc_module_channels_publish(&channels, &left, first) || memcmp(first, zero, sizeof(first)))
    {
        return 0;
    }
    enabled = 1;
    if (!styxrpc_module_channels_publish(&channels, &left, first) ||
        !styxrpc_module_channels_publish(&channels, &right, second) ||
        !memcmp(first, second, sizeof(first)) || styxrpc_module_channels_publish(&channels, &right, fresh))
    {
        return 0;
    }

    StyxRpcResource bus = styxrpc_module_channels_resource(&channels);
    void *x = NULL, *y = NULL, *invalid = NULL;
    size_t extent;
    unsigned char value = 0;
    unsigned access = STYXRPC_READ | STYXRPC_WRITE;
    int good = bus.open(bus.context, access, &x) && bus.open(bus.context, access, &y) &&
        bus.open(bus.context, access, &invalid);
    if (good)
    {
        good = !bus.read(x, 0, &value, 1, &extent) && !authorize(&bus, invalid, zero) &&
            !authorize(&bus, invalid, first) && authorize(&bus, x, first) && authorize(&bus, y, second) &&
            bus.write(x, STYXRPC_CHANNEL_TOKEN_SIZE, "a", 1, &extent) && extent == 1 &&
            bus.write(y, STYXRPC_CHANNEL_TOKEN_SIZE, "b", 1, &extent) &&
            a.writes == 1 && b.writes == 1 &&
            styxrpc_module_channels_revoke(&channels, first) && a.closes == 1 && b.closes == 0 &&
            styxrpc_module_channels_publish(&channels, &left, fresh) && memcmp(first, fresh, sizeof(first)) &&
            !styxrpc_module_channels_revoke(&channels, first) && !authorize(&bus, x, fresh) &&
            !bus.read(x, 0, &value, 1, &extent) && !bus.write(x, STYXRPC_CHANNEL_TOKEN_SIZE, "q", 1, &extent) &&
            bus.read(y, 0, &value, 1, &extent) && extent == 1 && value == 'b';
    }
    if (x)
    {
        bus.close(x);
    }
    if (y)
    {
        bus.close(y);
    }
    if (invalid)
    {
        bus.close(invalid);
    }
    return good && a.closes == 1 && b.closes == 1;
}


/* EN: Run the hosted bus regression without needing a VM or an entropy device.
 * RU: Выполнить регрессию шины без VM и системного источника энтропии. */
int main(void)
{
    if (!exercise())
    {
        fputs("module channel lease isolation failed\n", stderr);
        return 1;
    }
    return 0;
}
