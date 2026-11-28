/* Copyright (c) 2026 bhgv
 * EN: Shared current-key transform for verified channel frames.
 * RU: Общее преобразование текущих ключей проверенных кадров каналов.
 */
#ifndef STYXRPC_CHANNEL_REMAP_H
#define STYXRPC_CHANNEL_REMAP_H
#include "styxrpc/channel_route.h"

typedef struct ChannelKeyTransform
{
    int (*apply)(const void *context, uint32_t *key, int *unbuffered); /* Replace a checked key and capacity flag. */
    const void *context; /* Borrowed during reframing. */
    const unsigned char *token; /* Optional issued token for a result child. */
    int result;                 /* Reframe an output child instead of a message element. */
} ChannelKeyTransform;


/* EN: Apply mapping to current keys in one previously validated typed element.
 * RU: Применить mapping к текущим ключам ранее проверенного типизированного элемента. */
int styxrpc_channel_reframe(const ChannelKeyTransform *mapping, const void *bytes, size_t size,
    void *output, size_t capacity, size_t *used);

#endif
