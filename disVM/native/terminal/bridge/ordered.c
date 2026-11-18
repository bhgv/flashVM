/* Copyright (c) 2026 bhgv
 * EN: Captured launch-descriptor construction of negotiated generic terminal channels; no IDE-specific paths.
 * RU: Создание согласованных общих каналов по исходному launch-descriptor; без IDE-специфичных путей.
 */
#include "internal.h"


/* EN/RU: See ordered.h; common local queues/relays are shared with the existing flat bridge.
 * См. ordered.h; локальные очереди/relays общие с существующим flat-мостом.
 */
StyxBidirUIBridge *styxbidirordereduibridge(StyxOrdered *peer, unsigned local_side, unsigned long ui_tag)
{
    return uibridgecreate(NULL, peer, local_side, ui_tag);
}


/* EN/RU: Check complete native launch options before any fid walk/open or worker creation,1/0.
 * Проверить все native-настройки до walk/open и создания workers:1/0.
 */
static int valid(const StyxOrderedTerminalOptions *options)
{
    if (!options || options->struct_size != sizeof(*options) || options->local_side > DVM_UI_DIS ||
        options->limit > 65536U - 64U || !options->lane || options->lane > FVM_SRECORD_U32 ||
        !options->window || options->window >= (1ULL << 63) || !options->tags || !options->count ||
        options->ui_tag > FVM_SRECORD_U32)
    {
        return 0;
    }
    FVM_SInteger tag = {0, options->ui_tag, 0}; /* Explicit fixed inner tag, not an outer service. */
    unsigned char bytes[FVM_SRECORD_INTEGER_MAX]; /* Private canonical tag representation. */
    size_t used; /* Body bytes occupied by tag. */
    if (fvm_sinteger_write(&tag, bytes, sizeof(bytes), &used) != FVM_S_OK ||
        options->limit < DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX + used)
    {
        return 0;
    }
    int found = 0; /* UI grant must be declared explicitly by native host. */
    for (size_t i = 0; i < options->count; i++)
    {
        if (options->tags[i] > FVM_SRECORD_U32)
        {
            return 0;
        }
        found |= options->tags[i] == options->ui_tag;
        for (size_t j = 0; j < i; j++)
        {
            if (options->tags[j] == options->tags[i])
            {
                return 0;
            }
        }
    }
    return found;
}


/* EN/RU: See ordered.h; borrow options and captured authority only through construction, reserve outer framing explicitly.
 * См. ordered.h; заимствовать options и исходные полномочия лишь при создании, явно зарезервировать внешнее обрамление.
 */
StyxBidirUIBridge *styxbidirorderedterminal(StyxClient *client, const DVM_TerminalChannels *channels,
    const StyxOrderedTerminalOptions *options)
{
    if (!valid(options))
    {
        return NULL;
    }
    StyxBidirControl *control = styxbidircontrol(client, channels, options->limit + 64, 1); /* Captured capability, owned on success. */
    StyxOrdered *peer = styxbidirordered(control, options->limit, options->window, options->tags, options->count); /* Consumes control. */
    if (!peer)
    {
        return NULL;
    }
    StyxBidirUIBridge *bridge = styxbidirordereduibridge(peer, options->local_side, options->ui_tag); /* Consumes peer always. */
    if (!bridge)
    {
        return NULL;
    }
    if (!styxorderedoffer(peer, 1, options->lane))
    {
        styxbidiruibridgefree(bridge);
        return NULL;
    }
    return bridge;
}


/* EN/RU: See ordered.h; native host may service other grants without consuming the UI relay's queue.
 * См. ordered.h; native-host может обслуживать другие grants, не читая UI-очередь relay.
 */
StyxOrdered *styxbidiruibridgeordered(StyxBidirUIBridge *bridge)
{
    return bridge ? bridge->ordered : NULL;
}
