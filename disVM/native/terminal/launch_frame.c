/* Copyright (c) 2026 bhgv
 * EN: Decode one launch frame using the common S-record codec.
 * RU: Декодирование кадра запуска общим кодеком S-записей.
 */
#include "launch_frame.h"


/* EN/RU: See launch_frame.h for borrowed inputs, terminal ownership and I/O.
 * Параметры, владение terminal и I/O описаны в launch_frame.h. */
DVM_LaunchTicket *styxterminallaunchframe(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchEndpoint *endpoint,
    unsigned long tag, const void *bytes, size_t size)
{
    FVM_SRecord record; /* Local view borrowing bytes until submit returns. */
    size_t used; /* Complete encoded extent returned by the decoder. */
    if (fvm_srecord_read(bytes, size, DVM_LAUNCH_ARGUMENT_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &used) != FVM_S_OK || used != size)
    {
        styxclientfree(terminal);
        return NULL;
    }
    return styxbidirlaunchrecord(terminal, spec, access, paths, endpoint, tag, &record);
}
