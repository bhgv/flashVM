/* Copyright (c) 2026 bhgv
 * EN: Shared preflight and owned ordered connection from an exact captured channel descriptor.
 * RU: Общий preflight и owned ordered-соединение по точному закреплённому descriptor каналов.
 */
#include "attach.h"
#include "cancel.h"
#include "flashvm/sordctl.h"


/* EN/RU: See attach.h; encode tag privately to charge its exact original S-integer cost.
 * См. attach.h; кодировать tag локально для учёта точного размера исходного S-числа.
 */
int styxorderedoptionsvalid(const StyxOrderedOptions *options)
{
    if (!options || options->struct_size != sizeof(*options) || !options->limit || options->limit > 65536U - 64U ||
        !options->window || options->window >= (1ULL << 63) || !options->lane || options->lane > FVM_SRECORD_U32 ||
        (!options->tags && options->count) || options->count > (size_t)-1 / sizeof(*options->tags))
    {
        return 0;
    }
    for (size_t i = 0; i < options->count; i++)
    {
        FVM_SInteger tag = {0, options->tags[i], 0}; /* One application-local grant, not parent authority. */
        unsigned char encoded[FVM_SRECORD_INTEGER_MAX]; /* Private original integer bytes. */
        size_t used; /* Actual body overhead for this tag. */
        if (options->tags[i] > FVM_SRECORD_U32 ||
            fvm_sinteger_write(&tag, encoded, sizeof(encoded), &used) != FVM_S_OK || used > options->limit)
        {
            return 0;
        }
        for (size_t j = 0; j < i; j++)
        {
            if (options->tags[i] == options->tags[j])
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: See attach.h; tiny application bodies never reduce private service capacity below the codec maximum.
 * См. attach.h; малое тело клиента не уменьшает private service capacity ниже максимума кодека.
 */
StyxOrdered *styxbidirorderedchannels(StyxClient *client, const DVM_TerminalChannels *channels,
    const StyxOrderedOptions *options)
{
    if (!styxorderedoptionsvalid(options) || !dvm_terminal_channels_valid(channels) || !styxclientbidir(client) ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    unsigned outer_limit = options->limit + 64; /* Explicit framing reservation, independent native service minimum. */
    if (outer_limit < FVM_SORDER_CTL_MAX)
    {
        outer_limit = FVM_SORDER_CTL_MAX;
    }
    StyxBidirControl *control = styxbidircontrol(client, channels, outer_limit, 1); /* Captured authority, owned on success. */
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    StyxOrdered *peer = styxbidirordered(control, options->limit, options->window, options->tags, options->count); /* Consumes control always. */
    if (peer && (dvm_cancel_requested(dvm_cancel_current()) || !styxorderedoffer(peer, 1, options->lane) ||
        dvm_cancel_requested(dvm_cancel_current())))
    {
        styxorderedfree(peer);
        return NULL;
    }
    return peer;
}


/* EN/RU: See attach.h; wrapper never takes or rewrites the handoff and never enables a display implicitly.
 * См. attach.h; wrapper не поглощает/меняет handoff и не включает дисплей неявно.
 */
StyxOrdered *styxbidirorderedlaunch(StyxClient *client, const DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options)
{
    if (!handoff || !handoff->group || handoff->description.terminal_channels != 1)
    {
        return NULL;
    }
    return styxbidirorderedchannels(client, &handoff->flashvm, options);
}
