/* Copyright (c) 2026 bhgv
 * EN: Revision-checked namespace inspection through the retained-owner debugger gate.
 * RU: Namespace inspection с проверкой revision через debugger gate retained owner.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "runqueue.h"
#include "internal.h"
#include "vfs/observation.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define DVM_NS_COMMAND_VALUES 4 /* revision, binding index, field selector, byte offset. */


/* EN/RU: Parse exactly four unsigned decimal tokens after command/PID, without
 * narrowing/overflow/trailing junk. Разобрать четыре decimal tokens без overflow,
 * narrowing и trailing junk; caller owns output, one/zero, no allocation. */
static int arguments(const char *line, uint64_t *values)
{
    unsigned int token;
    for (token = 0; token < 2; token++)
    {
        while (*line && *line != ' ')
        {
            line++;
        }
        if (!*line)
        {
            return 0;
        }
        while (*line == ' ')
        {
            line++;
        }
    }
    for (token = 0; token < DVM_NS_COMMAND_VALUES; token++)
    {
        uint64_t value = 0;
        if (*line < '0' || *line > '9')
        {
            return 0;
        }
        while (*line >= '0' && *line <= '9')
        {
            unsigned int digit = (unsigned int)(*line++ - '0');
            if (value > (UINT64_MAX - digit) / 10)
            {
                return 0;
            }
            value = value * 10 + digit;
        }
        values[token] = value;
        if (token + 1 == DVM_NS_COMMAND_VALUES)
        {
            return !*line;
        }
        if (*line != ' ')
        {
            return 0;
        }
        while (*line == ' ')
        {
            line++;
        }
    }
    return 0;
}


/* EN: Read only native namespace under frozen target wake gate. Shared peers may
 * mutate their group; revision refusal prevents mixed pages. No GC inspector or I/O.
 * RU: Читать native namespace под wake gate frozen target. Shared peers могут
 * менять группу; revision refusal исключает смешение pages. Без GC inspector/I/O.
 * target/line/output borrowed; capacity bounds JSON; return through output only. */
void dvm_debug_namespace(DebugTarget *target, const char *line, char *output, size_t capacity)
{
    DVM_NamespaceObservation state;
    unsigned char bytes[DVM_NS_OBSERVE_BYTES];
    char hex[DVM_NS_OBSERVE_BYTES * 2 + 1];
    uint64_t values[DVM_NS_COMMAND_VALUES];
    int summary = !strncmp(line, "namespace ", 10);
    int result;
    size_t index;
    if (!target->native_ready || !target->requested || !target->process)
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"not retained\"}\n");
        return;
    }
    if (summary)
    {
        const char *pid = line + 10;
        while (*pid >= '0' && *pid <= '9')
        {
            pid++;
        }
        if (*pid)
        {
            snprintf(output, capacity, "{\"ok\":false,\"error\":\"invalid namespace request\"}\n");
            return;
        }
    }
    if (!summary && (!arguments(line, values) || values[1] > SIZE_MAX || values[2] > DVM_NS_FIELD_COMPONENT ||
                     values[3] > SIZE_MAX))
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"invalid namespace request\"}\n");
        return;
    }
    result = dvm_namespace_observe(((DVM_RuntimeProcess *)target->process)->namespace_group,
                                   summary ? NULL : &values[0],
                                   summary ? 0 : (size_t)values[1],
                                   summary ? 0 : (int)values[2],
                                   summary ? 0 : (size_t)values[3],
                                   bytes,
                                   sizeof(bytes),
                                   &state);
    if (result != DVM_NS_OBSERVED)
    {
        snprintf(output,
                 capacity,
                 "{\"ok\":false,\"error\":\"%s\"}\n",
                 result == DVM_NS_OBSERVE_STALE ? "namespace changed" : "invalid namespace request");
        return;
    }
    for (index = 0; index < state.copied; index++)
    {
        snprintf(hex + index * 2, 3, "%02x", bytes[index]);
    }
    hex[state.copied * 2] = 0;
    snprintf(
        output,
        capacity,
        "{\"ok\":true,\"revision\":\"%llu\",\"bindings\":%lu,\"references\":%lu,\"root_held\":%d,\"cwd_held\":%d,\"import_held\":%d,\"binding_held\":%d,\"length\":%lu,\"copied\":%lu,\"hex\":\"%s\"}\n",
        (unsigned long long)state.version,
        (unsigned long)state.bindings,
        (unsigned long)state.references,
        state.root_held,
        state.cwd_held,
        state.import_held,
        state.binding_held,
        (unsigned long)state.length,
        (unsigned long)state.copied,
        hex);
}
