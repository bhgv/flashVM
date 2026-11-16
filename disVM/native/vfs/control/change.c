/* Copyright (c) 2026 bhgv
 * EN: Two-phase versioned native feed replacement with nonwaiting cutover.
 * RU: Двухфазная versioned замена native feed с nonwaiting переключением.
 */
#include "internal.h"


/* EN/RU: See control.h; setup owns temporary client/journal pins and never holds control lock over I/O.
 * См. control.h; setup владеет temporary pins и не удерживает control lock при I/O. */
unsigned long long styxmountctlset(StyxMountCtl *control, unsigned long long expected,
                                 const StyxFeedConfig *config, StyxFeed **output)
{
    if (output)
    {
        *output = NULL;
    }
    if (!control || !config)
    {
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    int valid = !control->info.closed && !control->info.changing &&
        control->info.version == expected && expected != ~0ULL &&
        (control->access & FVM_ACCESS_READ);
    StyxClient *client = NULL;
    DVM_Changes *journal = NULL;
    if (valid && styxclientretain(control->client))
    {
        client = control->client;
        if (changesretain(control->journal))
        {
            journal = control->journal;
            control->info.changing = 1;
        }
    }
    pthread_mutex_unlock(&control->lock);
    if (!journal)
    {
        styxclientfree(client);
        return 0;
    }
    StyxFeed *candidate = styxfeedprepare(client, journal, config);
    StyxFeed *old = NULL;
    StyxFeed *retiring = NULL;
    unsigned kinds = config->kinds;
    unsigned long long result = 0;
    pthread_mutex_lock(&control->lock);
    if (candidate && !control->info.closed && control->info.version == expected &&
        styxfeedfollow(candidate, control->feed ? control->feed : control->retiring))
    {
        old = control->feed;
        retiring = control->retiring;
        control->retiring = NULL;
        if (old)
        {
            kinds |= old->config.kinds;
        }
        styxfeedactivate(candidate);
        control->feed = candidate;
        control->info.configured = 1;
        result = ++control->info.version;
        if (output)
        {
            styxfeedretain(candidate);
            *output = candidate;
        }
    }
    control->info.changing = 0;
    pthread_mutex_unlock(&control->lock);
    if (result)
    {
        styxfeedfree(old);
        styxfeedfree(retiring);
        changespost(journal, kinds|STYX_EVENT_CONNECTION, 0, 0);
    }
    else
    {
        styxfeedstop(candidate);
        styxfeedfree(candidate);
    }
    styxclientfree(client);
    dvm_changes_destroy(journal);
    return result;
}


/* EN/RU: See control.h; a concurrent preparation loses its CAS while keeping its private cleanup pins.
 * См. control.h; concurrent preparation теряет CAS, сохраняя private cleanup pins. */
unsigned long long styxmountctlclear(StyxMountCtl *control, unsigned long long expected)
{
    if (!control)
    {
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    StyxFeed *old = NULL;
    DVM_Changes *journal = NULL;
    unsigned long long result = 0;
    if (!control->info.closed && control->info.version == expected && expected != ~0ULL &&
        changesretain(control->journal))
    {
        journal = control->journal;
        if (control->feed)
        {
            old = control->retiring;
            control->retiring = control->feed;
        }
        control->feed = NULL;
        control->info.configured = 0;
        result = ++control->info.version;
        styxfeedstop(control->retiring);
    }
    pthread_mutex_unlock(&control->lock);
    if (result)
    {
        unsigned kinds = STYX_EVENT_ALL;
        styxfeedfree(old);
        changespost(journal, kinds|STYX_EVENT_CONNECTION, 0, 0);
        dvm_changes_destroy(journal);
    }
    return result;
}
