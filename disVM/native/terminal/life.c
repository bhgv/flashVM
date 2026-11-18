/* Copyright (c) 2026 bhgv
 * EN: Bidir control construction and joined native lifetime, preserving the host's session owner.
 * RU: Создание bidir control и joined native lifetime с сохранением host session owner.
 */
#include "internal.h"
#include "mount_grant.h"
#include "mount_cancel.h"


/* EN/RU: See control.h; signal cancellation only, retain all storage through explicit join/free.
 * См. control.h; только cancellation, вся storage живёт до join/free. */
void styxbidircontrolstop(StyxBidirControl *control)
{
    if (control)
    {
        controlend(control, STYX_CONTROL_STOPPED);
    }
}


/* EN/RU: See control.h; external callers retire first; never free on workers or retirement callbacks.
 * См. control.h; сначала завершить callers, не free из workers/retirement callbacks. */
void styxbidircontrolfree(StyxBidirControl *control)
{
    if (control)
    {
        styxbidircontrolstop(control);
        for (unsigned i = 0; i < control->started; i++)
        {
            pthread_join(control->workers[i], NULL);
        }
        controllinkdetach(control);
        styxmountclunk(control->client, control->rx);
        styxmountclunk(control->client, control->tx);
        styxmountclunk(control->client, control->root);
        styxclientfree(control->client);
        dvm_cancel_release(control->retired);
        free(control->incoming.bytes);
        free(control->outgoing.bytes);
        free(control->decode);
        pthread_cond_destroy(&control->changed);
        pthread_mutex_destroy(&control->lock);
        free(control);
    }
}


/* EN/RU: See control.h; allocation and root-relative endpoint setup precede native publication.
 * См. control.h; allocation и endpoint setup относительно grant завершаются до publication. */
StyxBidirControl *styxbidircontrol(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots)
{
    if (!styxclientbidir(client) || !styxclientwatchable(client) || !dvm_terminal_channels_valid(channels) ||
        !limit || limit > 65536 || !slots || slots > 32 || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    StyxBidirControl *control = calloc(1, sizeof(*control));
    if (!control)
    {
        return NULL;
    }
    if (pthread_mutex_init(&control->lock, NULL))
    {
        free(control);
        return NULL;
    }
    pthread_condattr_t attributes;
    int initialized = pthread_condattr_init(&attributes) == 0;
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&control->changed, &attributes);
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&control->lock);
        free(control);
        return NULL;
    }
    control->retired = dvm_cancel_create();
    control->limit = limit;
    control->slots = slots;
    control->extent = limit+32;
    control->incoming.bytes = malloc(control->extent*slots);
    control->outgoing.bytes = malloc(control->extent*slots);
    control->decode = malloc(control->extent);
    if (!control->retired || !control->incoming.bytes || !control->outgoing.bytes || !control->decode || !styxclientretain(client))
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    control->client = client;
    control->root = styxmountgrantfid(channels->provider, client, FVM_ACCESS_READ | FVM_ACCESS_WRITE);
    StyxFid *base = control->root; /* Borrowed immutable base; control retains it for later resource walks. */
    if (base && (styxclientfiddirectory(base) != 0 ||
        (!strcmp(channels->receive, ".") && !strcmp(channels->send, "."))))
    {
        control->rx = styxmountwalkcancel(client, base, channels->receive);
        control->tx = styxmountwalkcancel(client, base, channels->send);
    }
    if (!control->rx || !control->tx || styxclientfiddirectory(control->rx) != 0 ||
        styxclientfiddirectory(control->tx) != 0 || !styxmountopen(client, control->rx, 0) ||
        !styxmountopen(client, control->tx, 1) || dvm_cancel_requested(dvm_cancel_current()))
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    if (!controllinkattach(control) || styxbidircontrolstate(control))
    {
        styxbidircontrolfree(control);
        return NULL;
    }
    if (!pthread_create(&control->workers[0], NULL, controlreceive, control))
    {
        control->started = 1;
        if (!styxbidircontrolstate(control) && !dvm_cancel_requested(dvm_cancel_current()) &&
            !pthread_create(&control->workers[1], NULL, controltransmit, control))
        {
            control->started = 2;
            if (!styxbidircontrolstate(control) && !dvm_cancel_requested(dvm_cancel_current()))
            {
                return control;
            }
        }
    }
    styxbidircontrolfree(control);
    return NULL;
}
