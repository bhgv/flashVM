/* Copyright (c) 2026 bhgv
 * EN: Bounded local debugger commands operate only on native registry publications.
 * RU: Bounded local debugger commands работают только с native registry publications.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include "managed_workers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>

extern int dvm_runtime_request_guest_cancel(int pid); /* Existing unbound-host cancellation path. */


/* EN/RU: Quote bounded copied text as JSON; no managed string/heap access.
 * Экранировать bounded copied text как JSON; без managed string/heap access. */
static void quote(const char *source, char *output, size_t capacity)
{
    size_t used = 0;
    while (*source && used + 7 < capacity)
    {
        unsigned char value = (unsigned char)*source++;
        if (value == '"' || value == '\\')
        {
            output[used++] = '\\';
            output[used++] = (char)value;
        }
        else if (value < 32)
        {
            used += (size_t)snprintf(output + used, capacity - used, "\\u%04x", value);
        }
        else if (value < 128)
        {
            output[used++] = (char)value;
        }
        else
        {
            unsigned int width = value >= 0xc2 && value <= 0xdf   ? 2
                                 : value >= 0xe0 && value <= 0xef ? 3
                                 : value >= 0xf0 && value <= 0xf4 ? 4
                                                                  : 0;
            unsigned int at;
            int valid = width != 0;
            for (at = 1; valid && at < width; at++)
            {
                unsigned char next = (unsigned char)source[at - 1];
                if (!next || (next & 0xc0) != 0x80 ||
                    (at == 1 && ((value == 0xe0 && next < 0xa0) || (value == 0xed && next >= 0xa0) ||
                                 (value == 0xf0 && next < 0x90) || (value == 0xf4 && next >= 0x90))))
                {
                    valid = 0;
                }
            }
            if (valid)
            {
                output[used++] = (char)value;
                memcpy(output + used, source, width - 1);
                used += width - 1;
                source += width - 1;
            }
            else
            {
                memcpy(output + used, "\\ufffd", 6);
                used += 6;
            }
        }
    }
    output[used] = 0;
}


/* EN/RU: Format a copied publication; running data is explicitly not a live register view.
 * Форматировать copied publication; running data явно не live register view. */
static void status(DebugTarget *target, int registers, char *output, size_t capacity)
{
    char error[768];
    char name[384];
    char path[1536];
    quote(target->error, error, sizeof(error));
    quote(target->location.name, name, sizeof(name));
    quote(target->location.path, path, sizeof(path));
    if (registers)
    {
        REG *r = &target->registers;
        snprintf(
            output,
            capacity,
            "{\"ok\":true,\"pid\":%d,\"retained\":%s,\"PC\":\"%llx\",\"FP\":\"%llx\",\"SP\":\"%llx\",\"TS\":\"%llx\",\"EX\":\"%llx\",\"M\":\"%llx\",\"MP\":\"%llx\",\"S\":\"%llx\",\"D\":\"%llx\",\"MID\":\"%llx\",\"IC\":%d,\"T\":%lld,\"ST\":%lld,\"DT\":%lld}\n",
            target->pid,
            target->native_ready && target->requested ? "true" : "false",
            (unsigned long long)(uintptr)r->PC,
            (unsigned long long)(uintptr)r->FP,
            (unsigned long long)(uintptr)r->SP,
            (unsigned long long)(uintptr)r->TS,
            (unsigned long long)(uintptr)r->EX,
            (unsigned long long)(uintptr)r->M,
            (unsigned long long)(uintptr)r->MP,
            (unsigned long long)(uintptr)r->s,
            (unsigned long long)(uintptr)r->d,
            (unsigned long long)(uintptr)r->m,
            r->IC,
            (long long)r->t,
            (long long)r->st,
            (long long)r->dt);
    }
    else
    {
        snprintf(
            output,
            capacity,
            "{\"ok\":true,\"pid\":%d,\"state\":\"%s\",\"requested\":%s,\"retained\":%s,\"steps\":%lu,\"fault_stop_refused\":%s,\"module\":\"%s\",\"path\":\"%s\",\"pc\":%d,\"opcode\":%u,\"error\":\"%s\"}\n",
            target->pid,
            target->state == DebugFaulted  ? "faulted"
            : target->state == DebugPaused ? "paused"
                                           : "running",
            target->requested ? "true" : "false",
            target->native_ready && target->requested ? "true" : "false",
            target->steps,
            target->refused ? "true" : "false",
            name,
            path,
            target->location.next_pc,
            target->instruction.op,
            error);
    }
}


/* EN/RU: Format copied CPU context only; raw addresses grant no read capability.
 * Форматировать только copied CPU context; raw addresses не дают read capability. */
static void native_registers(const DebugTarget *target, char *output, size_t capacity)
{
    const DVM_JitFault *fault = &target->native_fault;
    size_t used;
    unsigned int index;
    if (!fault->signal_number)
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"no native fault context\"}\n");
        return;
    }
    used = (size_t)snprintf(output, capacity,
        "{\"ok\":true,\"signal\":%d,\"code\":%d,\"address\":\"%llx\",\"pc\":\"%llx\",\"sp\":\"%llx\",\"rx_offset\":%llu,\"architecture\":\"%s\",\"registers\":{",
        fault->signal_number, fault->signal_code, (unsigned long long)fault->address,
        (unsigned long long)fault->pc, (unsigned long long)fault->sp,
        (unsigned long long)(fault->pc - fault->rx_base), dvm_jit_fault_architecture());
    for (index = 0; index < fault->count && index < 32 && used + 32 < capacity; index++)
    {
        used += (size_t)snprintf(output + used, capacity - used, "%s\"%s\":\"%llx\"",
                               index ? "," : "", dvm_jit_fault_register_name(index), (unsigned long long)fault->registers[index]);
    }
    snprintf(output + used, capacity - used, "}}\n");
}


/* EN: One local authenticated-by-socket command. Registry pins target, then wake
 * serializes its publication/gate. Socket I/O occurs after all these locks release.
 * RU: Одна local command с socket permissions. Registry удерживает target, wake
 * защищает publication/gate. Socket I/O идёт после снятия всех этих locks. */
void dvm_debug_command(const char *line, char *output, size_t capacity)
{
    char command[24];
    char extra;
    int pid;
    int consumed = 0;
    long parsed;
    char number[24];
    char *end;
    int wake = 0;
    int terminate = 0;
    DebugTarget *target;
    snprintf(output, capacity, "{\"ok\":false,\"error\":\"invalid command\"}\n");
    if (!strcmp(line, "hello 0"))
    {
        snprintf(output,
                 capacity,
                 "{\"ok\":true,\"session\":\"%s\",\"host_pid\":%ld}\n",
                 dvm_debug_service.session,
                 (long)getpid());
        return;
    }
    if (strlen(line) < 34 || line[0] != '@' || memcmp(line + 1, dvm_debug_service.session, 32) || line[33] != ' ')
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"stale session\"}\n");
        return;
    }
    line += 34;
    if (sscanf(line, "%23s %23s%n", command, number, &consumed) != 2 ||
        (line[consumed] && !isspace((unsigned char)line[consumed])) || !isdigit((unsigned char)number[0]))
    {
        return;
    }
    errno = 0;
    parsed = strtol(number, &end, 10);
    if (errno || *end || parsed < 0 || parsed > INT_MAX)
    {
        return;
    }
    pid = (int)parsed;
    fvm_port_sync_lock(dvm_debug_service.lock);
    if (!strcmp(command, "list"))
    {
        DebugTarget *selected = NULL;
        for (target = dvm_debug_service.targets; target; target = target->next)
        {
            if (target->pid > pid && (!selected || target->pid < selected->pid))
            {
                selected = target;
            }
        }
        pid = selected ? selected->pid : 0;
        snprintf(output, capacity, "{\"ok\":true,\"pid\":%d}\n", pid);
        fvm_port_sync_unlock(dvm_debug_service.lock);
        return;
    }
    for (target = dvm_debug_service.targets; target && target->pid != pid; target = target->next)
    {
    }
    if (!target)
    {
        snprintf(output, capacity, "{\"ok\":false,\"error\":\"gone\"}\n");
        fvm_port_sync_unlock(dvm_debug_service.lock);
        return;
    }
    fvm_port_sync_lock(target->wake);
    if ((!strcmp(command, "pause") || !strcmp(command, "continue") || !strcmp(command, "terminate")) &&
        sscanf(line, "%23s %d %c", command, &pid, &extra) == 2)
    {
        if (!strcmp(command, "pause"))
        {
            target->requested = 1;
            if (target->native_ready)
            {
                target->state = target->fault ? DebugFaulted : DebugPaused;
            }
        }
        else
        {
            target->requested = 0;
            target->state = DebugRunning;
            terminate = !strcmp(command, "terminate");
            target->terminate |= terminate;
        }
        fvm_port_sync_signal_all(target->wake);
        wake = 1;
        snprintf(output, capacity, "{\"ok\":true,\"accepted\":true}\n");
    }
    else if (!strcmp(command, "step"))
    {
        if (target->native_ready && target->requested && !target->fault && target->boundary)
        {
            target->step_after = 1;
            target->requested = 0;
            target->state = DebugRunning;
            fvm_port_sync_signal_all(target->wake);
            snprintf(output, capacity, "{\"ok\":true,\"accepted\":true}\n");
        }
        else
        {
            snprintf(output, capacity, "{\"ok\":false,\"error\":\"step requires instruction-boundary pause\"}\n");
        }
    }
    else if (!strcmp(command, "native-registers"))
    {
        native_registers(target, output, capacity);
    }
    else if (!strcmp(command, "operation"))
    {
        unsigned long long source = 0;
        unsigned long long destination = 0;
        memcpy(&source, &target->instruction.s, sizeof(source));
        memcpy(&destination, &target->instruction.d, sizeof(destination));
        snprintf(
            output,
            capacity,
            "{\"ok\":true,\"opcode\":%u,\"address_mode\":%u,\"reg\":%u,\"source\":\"%llx\",\"destination\":\"%llx\"}\n",
            target->instruction.op,
            target->instruction.add,
            target->instruction.reg,
            source,
            destination);
    }
    else if (!strcmp(command, "status") || !strcmp(command, "registers"))
    {
        status(target, !strcmp(command, "registers"), output, capacity);
    }
    else
    {
        dvm_debug_inspect(target, line, output, capacity);
    }
    fvm_port_sync_unlock(target->wake);
    fvm_port_sync_unlock(dvm_debug_service.lock);
    if (wake)
    {
        dvm_managed_workers_debug_wake(pid);
        if (terminate)
        {
            dvm_runtime_request_guest_cancel(pid);
        }
    }
}
