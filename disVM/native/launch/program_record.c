/* Copyright (c) 2026 bhgv
 * EN: Bounded versioned process selector for an opt-in Styx launch record.
 * RU: Ограниченный версионированный выбор процесса в специальной записи запуска Styx.
 */
#include "program_record.h"

#define PROGRAM_VERSION 1U /* First process-selector format. */


/* EN/RU: Encode a valid selector into the caller's thirteen-byte buffer.
 * Записать допустимый выбор процесса в буфер вызывающего из тринадцати байт. */
int styxlaunchprogramencode(void *header, const DVM_LaunchProgram *program)
{
    if (!header || !program || !program->pid || program->pid == program->parent_pid)
    {
        return 0;
    }
    unsigned char *out = header;
    const uint32_t fields[] = {program->pid, program->parent_pid, program->hidden_fds};
    *out++ = PROGRAM_VERSION;
    for (size_t i = 0; i < 3; i++)
    {
        for (unsigned byte = 0; byte < 4; byte++)
        {
            *out++ = (unsigned char)(fields[i] >> (24 - byte * 8));
        }
    }
    return 1;
}


/* EN/RU: Decode within the payload extent and leave the caller unchanged on refusal.
 * Прочитать в пределах данных, оставив результат неизменным при отказе. */
int styxlaunchprogramdecode(const void *bytes, size_t size, DVM_LaunchProgram *program)
{
    const unsigned char *in = bytes;
    if (!in || !program || size < STYX_LAUNCH_PROGRAM_HEADER || *in++ != PROGRAM_VERSION)
    {
        return 0;
    }
    uint32_t fields[3] = {0};
    for (size_t i = 0; i < 3; i++)
    {
        for (unsigned byte = 0; byte < 4; byte++)
        {
            fields[i] = (fields[i] << 8) | *in++;
        }
    }
    if (!fields[0] || fields[0] == fields[1])
    {
        return 0;
    }
    *program = (DVM_LaunchProgram){fields[0], fields[1], fields[2]};
    return 1;
}
