/* Copyright (c) 2026 bhgv
 * EN: Stop real AVM1 playback and inspect or advance it using paired .state fids.
 * RU: Останавливать AVM1 и просматривать либо продвигать выполнение через fid .state.
 */
#define _POSIX_C_SOURCE 200809L /* Expose the counted snprintf declaration in C89 mode. */
#include "playback_internal.h"
#include "flashvm/debug.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>


/* EN: Copy a counted label into a .state line without admitting controls or raw newlines.
 * RU: Скопировать подпись с длиной в строку .state без управляющих символов и переводов. */
static void copy_label(char *to, size_t capacity, const char *from, size_t size)
{
    size_t i;
    if (size >= capacity)
    {
        size = capacity - 1;
    }
    for (i = 0; i < size; ++i)
    {
        unsigned char byte = (unsigned char)from[i];
        to[i] = byte >= ' ' && byte < 127 ? (char)byte : '?';
    }
    to[size] = 0;
}


/* EN: Format the live AVM1 location and bounded innermost frames into the owner's buffer.
 * RU: Записать текущую позицию AVM1 и ограниченный внутренний стек в буфер владельца. */
static int capture(PlaybackDebug *debug, FVM_Context *context, const FVM_DebugBreakpoint *point)
{
    char function[PLAYBACK_DEBUG_FUNCTION_MAX + 1], source[PLAYBACK_DEBUG_FUNCTION_MAX + 1];
    size_t frames, i;
    int count;
    if (fvm_debug_frame_count(context, &frames) != FVM_OK)
    {
        return 0;
    }
    copy_label(function, sizeof(function), point->function, strlen(point->function));
    copy_label(source, sizeof(source), point->source, strlen(point->source));
    count = snprintf(debug->snapshot, sizeof(debug->snapshot),
        "debug.state=stopped\ndebug.stop=%lu\ndebug.offset=%lu\ndebug.line=%lu\ndebug.function=%s\n"
        "debug.source=%s\ndebug.frames=%lu\n",
        debug->stop, point->offset, point->line, function, source, (unsigned long)frames);
    if (count < 0 || (size_t)count >= sizeof(debug->snapshot))
    {
        return 0;
    }
    debug->length = (size_t)count;
    for (i = 0; i < frames && i < PLAYBACK_DEBUG_FRAMES_MAX; ++i)
    {
        FVM_DebugFrame frame;
        size_t capacity = sizeof(debug->snapshot) - debug->length;
        if (fvm_debug_frame_at(context, i, &frame) != FVM_OK)
        {
            return 0;
        }
        copy_label(function, sizeof(function), frame.function, frame.function_size);
        count = snprintf(debug->snapshot + debug->length, capacity, "debug.frame.%lu=%lu %d %s\n",
            (unsigned long)i, frame.offset, (int)frame.engine, function);
        if (count < 0 || (size_t)count >= capacity)
        {
            return 0;
        }
        debug->length += (size_t)count;
    }
    return 1;
}


/* EN: Keep the timeline activation alive, running only control I/O until resume or link loss.
 * RU: Удерживать активацию шкалы, обрабатывая только управление до продолжения либо обрыва. */
static FVM_Result breakpoint(FVM_Context *context, const FVM_DebugBreakpoint *point, void *data)
{
    PlaybackDebug *debug = data;
    if (!debug->ticking)
    {
        return FVM_OK;
    }
    if (debug->stop == ULONG_MAX)
    {
        return FVM_UNSUPPORTED;
    }
    ++debug->stop;
    if (!capture(debug, context, point))
    {
        return FVM_ERROR;
    }
    debug->resume = 0;
    debug->stopped = 1;
    while (!debug->resume && styxrpc_server_step_control(debug->server))
    {
    }
    debug->stopped = 0;
    return debug->resume ? FVM_OK : FVM_ERROR;
}


/* EN: Parse a decimal prefix, returning its byte count or zero for bad syntax/overflow.
 * RU: Разобрать десятичный префикс и вернуть размер либо ноль при ошибке/переполнении. */
static size_t decimal(const char *bytes, size_t size, unsigned long *out)
{
    size_t i = 0;
    unsigned long value = 0;
    while (i < size && bytes[i] >= '0' && bytes[i] <= '9')
    {
        unsigned long digit = (unsigned long)(bytes[i] - '0');
        if (value > (ULONG_MAX - digit) / 10UL)
        {
            return 0;
        }
        value = 10UL * value + digit;
        ++i;
    }
    *out = value;
    return i;
}


/* EN: Interpret a debugger-only command. The stop ID must match the active stop on resume.
 * RU: Обработать команду отладчика. Номер для продолжения должен совпасть с активным. */
static int debug_command(void *data, const char *bytes, size_t size)
{
    static const char *const words[] = {"break ", "clear ", "continue ", "step "};
    enum { SET, CLEAR, CONTINUE, STEP };
    FVM_StyxRpcPlayback *playback = data;
    PlaybackDebug *debug = &playback->debug;
    size_t verb, prefix, digits;
    unsigned long value;
    char function[PLAYBACK_DEBUG_FUNCTION_MAX + 1];
    if (!debug->attached)
    {
        return 0;
    }
    for (verb = 0; verb < sizeof(words) / sizeof(*words); ++verb)
    {
        prefix = strlen(words[verb]);
        if (size > prefix && !memcmp(bytes, words[verb], prefix))
        {
            break;
        }
    }
    if (verb >= sizeof(words) / sizeof(*words))
    {
        return 0;
    }
    bytes += prefix;
    size -= prefix;
    digits = decimal(bytes, size, &value);
    if (!digits)
    {
        return 0;
    }
    if (verb == CONTINUE || verb == STEP)
    {
        if (!debug->stopped || debug->resume || value != debug->stop || digits != size ||
            (verb == STEP && fvm_debug_step_action(playback->context) != FVM_OK))
        {
            return 0;
        }
        debug->resume = 1;
        return 1;
    }
    size -= digits;
    bytes += digits;
    if (size)
    {
        if (*bytes != ' ' || size < 2 || size > sizeof(function) ||
            memchr(bytes + 1, 0, size - 1) || memchr(bytes + 1, '\n', size - 1))
        {
            return 0;
        }
        memcpy(function, bytes + 1, size - 1);
        --size;
    }
    function[size] = 0;
    return fvm_debug_set_breakpoint(playback->context, function, value, verb == SET) == FVM_OK;
}


/* EN/RU: See playback_debug.h. / См. playback_debug.h. */
void fvm_styxrpc_debug_init(FVM_StyxRpcPlayback *playback, StyxRpcServer *server)
{
    playback->debug.server = server;
    if (server)
    {
        styxrpc_control_set_extension(playback->control, debug_command);
    }
}


/* EN/RU: See playback_debug.h. / См. playback_debug.h. */
FVM_Result fvm_styxrpc_debug_attach(FVM_StyxRpcPlayback *playback)
{
    PlaybackDebug *debug = &playback->debug;
    FVM_Result result;
    if (!debug->server || debug->attached)
    {
        return FVM_OK;
    }
    result = fvm_debug_set_breakpoint_handler(playback->context, breakpoint, debug);
    debug->attached = result == FVM_OK;
    return result;
}


/* EN/RU: See playback_debug.h. / См. playback_debug.h. */
size_t fvm_styxrpc_debug_sample(const PlaybackDebug *debug, char *bytes, size_t capacity)
{
    static const char armed[] = "debug.state=armed\n";
    size_t length;
    if (!debug->attached)
    {
        return 0;
    }
    length = debug->stopped ? debug->length : sizeof(armed) - 1;
    if (length <= capacity)
    {
        memcpy(bytes, debug->stopped ? debug->snapshot : armed, length);
        return length;
    }
    return 0;
}
