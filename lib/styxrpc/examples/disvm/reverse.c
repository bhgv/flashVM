/* Copyright (c) 2026 bhgv
 * EN: Register a C-owned process with a variable, numbered FDs, and virtual files.
 * RU: Регистрация процесса C с переменной, номерными FD и виртуальными файлами.
 */
#include "reverse.h"
#include <string.h>

static const char input[] = "I";    /* One byte available on read-only FD 0. */
static const char marker[] = "C\n"; /* Caller-owned virtual namespace content. */


/* EN: Load one live signed value from the C application.
 * RU: Прочитать живой int из приложения C. */
static int load_value(void *context, int32_t *value)
{
    *value = ((ReverseState *)context)->value;
    return 1;
}


/* EN: Store one validated signed value into the C application.
 * RU: Записать проверенный int в приложение C. */
static int store_value(void *context, int32_t value)
{
    ((ReverseState *)context)->value = value;
    return 1;
}


/* EN: Serve a positional slice of the borrowed string resource.
 * RU: Выдать позиционный фрагмент заимствованного строкового ресурса. */
static int read_text(void *context, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const char *source = context;
    size_t length = strlen(source);
    size_t remaining = offset < length ? length - (size_t)offset : 0;
    *size = capacity < remaining ? capacity : remaining;
    if (*size)
    {
        memcpy(bytes, source + (size_t)offset, *size);
    }
    return 1;
}


/* EN: Acknowledge one byte on the C-owned output slot.
 * RU: Подтвердить запись одного байта в слот вывода C. */
static int write_byte(void *context, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    if (offset || count != 1)
    {
        return 0;
    }
    *(unsigned char *)context = *(const unsigned char *)bytes;
    *size = count;
    return 1;
}


/* EN: Notify the C caller after the Dis child reaches its cancellation checkpoint.
 * RU: Уведомить C-клиента после достижения точки проверки отмены в Dis. */
static int write_checkpoint(void *context, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    if (offset || count != 1 || *(const char *)bytes != 'P')
    {
        return 0;
    }
    ReverseState *state = context;
    __atomic_store_n(&state->parked, 1, __ATOMIC_RELEASE);
    *size = count;
    return 1;
}


/* EN/RU: See reverse.h. / См. reverse.h. */
int register_reverse(ReverseState *state)
{
    const StyxIntVariable count = {state, load_value, store_value};
    const StyxRpcResource stdin_file = {.context = (void *)input, .read = read_text};
    const StyxRpcResource marker_file = {.context = (void *)marker, .read = read_text};
    const StyxRpcResource stdout_file = {.context = &state->output, .write = write_byte};
    const StyxRpcResource descriptor = {.context = &state->byte, .write = write_byte};
    const StyxRpcResource checkpoint = {.context = state, .write = write_checkpoint};
    StyxRpcPrograms *tree = &state->tree;
    return styxrpc_programs_init(tree, state->nodes, sizeof(state->nodes) / sizeof(*state->nodes)) &&
        styxrpc_program_add(tree, 42) && !styxrpc_program_add(tree, 42) &&
        styxrpc_program_fd(tree, 42, 0, STYXRPC_READ, sizeof(input) - 1, &stdin_file) &&
        !styxrpc_program_fd(tree, 42, 0, STYXRPC_READ, sizeof(input) - 1, &stdin_file) &&
        styxrpc_program_fd(tree, 42, 1, STYXRPC_WRITE, 0, &stdout_file) &&
        styxrpc_program_fd(tree, 42, 7, STYXRPC_WRITE, 0, &descriptor) &&
        styxrpc_program_int(tree, 42, "count", &count) &&
        styxrpc_program_file(tree, 42, "marker", STYXRPC_READ, sizeof(marker) - 1, &marker_file) &&
        styxrpc_program_file(tree, 42, "parked", STYXRPC_WRITE, 0, &checkpoint);
}


/* EN/RU: See reverse.h. / См. reverse.h. */
void *run_reverse(void *context)
{
    ReverseState *state = context;
    while (styxrpc_server_step(state->server))
    {
    }
    styxrpc_server_destroy(state->server);
    return NULL;
}
