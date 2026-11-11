/* Copyright (c) 2026 bhgv
 * EN: Serialize a peer match with concurrent buffer reads and writes.
 * RU: Согласовать сопоставление участника с параллельными чтениями/записями буфера.
 */
#include "channel_match.h"
#include <stdlib.h>

static DVM_ChannelMatch *matches; /* All accesses require the channel queue mutex. */


/* EN: Publish a borrowed stack ledger after checking the channel is unclaimed.
 * RU: Опубликовать заимствованную стековую запись, проверив отсутствие владельца. */
int dvm_channel_match_begin(DVM_ChannelMatch *match, void *channel, void *owner)
{
    for (DVM_ChannelMatch *entry = matches; entry; entry = entry->next)
    {
        if (entry->channel == channel)
        {
            return 0;
        }
    }
    match->channel = channel;
    match->owner = owner;
    match->next = matches;
    matches = match;
    return 1;
}


/* EN: Compare borrowed channel and owner identities under the queue mutex.
 * RU: Сравнить заимствованные channel/owner под мьютексом очереди. */
int dvm_channel_match_blocked(void *channel, void *owner)
{
    for (DVM_ChannelMatch *entry = matches; entry; entry = entry->next)
    {
        if (entry->channel == channel && entry->owner != owner)
        {
            return 1;
        }
    }
    return 0;
}


/* EN: Remove caller's exact ledger, aborting on broken acquisition pairing.
 * RU: Удалить точную запись вызывающего, завершить при нарушении парности. */
void dvm_channel_match_end(DVM_ChannelMatch *match)
{
    DVM_ChannelMatch **link = &matches;
    while (*link && *link != match)
    {
        link = &(*link)->next;
    }
    if (!*link)
    {
        abort();
    }
    *link = match->next;
    match->next = NULL;
    match->channel = NULL;
    match->owner = NULL;
}
