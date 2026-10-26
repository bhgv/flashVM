/* Produce one real broker deviation for the language-channel examples; no network connection is opened. */
#include "broker.h"
#include "codec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* This executable is a single serialized owner, so its supplied lock has no contending users. */
static void serialized(void *unused)
{
    (void)unused;
}


/* Parse one bounded decimal scalar without signs, narrowing or unchecked overflow. */
static int decimal(const char *text, uint64_t limit, uint64_t *out)
{
    uint64_t value = 0;
    if (!text || !text[0])
    {
        return 0;
    }
    while (*text)
    {
        unsigned digit = (unsigned char)*text++;
        if (digit < '0' || digit > '9' || value > (limit - (digit - '0')) / 10)
        {
            return 0;
        }
        value = value * 10 + digit - '0';
    }
    *out = value;
    return 1;
}


/* Register exact compiler coordinates, publish an owner wait reason and emit its slow diagnostic.
 * Optional argv supplies path,line,column,offset,length,binding,build from FIRDBG1.
 * All streams/native metadata are owned here; zero means one complete counted BD1 record was written. */
int main(int argc, char **argv)
{
    DVM_BrokerSync sync = {serialized, serialized, NULL};
    DVM_Broker *broker = NULL;
    DVM_BrokerSite source;
    DVM_BrokerChannel card;
    DVM_BrokerSnapshot request;
    DVM_BrokerEvent event;
    DVM_BrokerKey key;
    uint32_t site;
    uint32_t channel;
    uint32_t listener;
    uint64_t dropped;
    uint64_t numbers[5];
    char frame[DVM_BROKER_FRAME_MAX];
    size_t size;
    int index;
    int ok = 0;
    memset(&source, 0, sizeof(source));
    memset(&card, 0, sizeof(card));
    memset(&request, 0, sizeof(request));
    strcpy(source.build, "broker-example");
    strcpy(source.path, "source/пример.as");
    strcpy(source.module, "Example.value");
    source.line = 3;
    source.column = 17;
    source.offset = 100;
    source.length = 1;
    source.binding = 1;
    if (argc != 1 && argc != 9)
    {
        fprintf(stderr, "usage: broker-frame [PATH LINE COLUMN OFFSET LENGTH BINDING BUILD MODULE]\n");
        return 2;
    }
    if (argc == 9)
    {
        if (strlen(argv[1]) >= sizeof(source.path) || strlen(argv[7]) >= sizeof(source.build) ||
            strlen(argv[8]) >= sizeof(source.module))
        {
            return 2;
        }
        for (index = 0; index < 5; index++)
        {
            if (!decimal(argv[index + 2], index == 2 ? UINT64_MAX : UINT32_MAX, &numbers[index]))
            {
                return 2;
            }
        }
        strcpy(source.path, argv[1]);
        strcpy(source.build, argv[7]);
        strcpy(source.module, argv[8]);
        source.line = (uint32_t)numbers[0];
        source.column = (uint32_t)numbers[1];
        source.offset = numbers[2];
        source.length = (uint32_t)numbers[3];
        source.binding = (uint32_t)numbers[4];
    }
    if (dvm_broker_create(&sync, &broker) != DVM_BROKER_OK ||
        dvm_broker_site(broker, &source, &site) != DVM_BROKER_OK ||
        dvm_broker_watch(broker, 0, 0, &listener) != DVM_BROKER_OK)
    {
        goto done;
    }
    card.transport = DVM_BROKER_NETWORK;
    strcpy(card.logical, "ai.primary");
    strcpy(card.peer, "tcp://127.0.0.1:9001");
    card.created_at = site;
    card.connected_at = site;
    if (dvm_broker_open(broker, &card, 0, &channel) != DVM_BROKER_OK)
    {
        goto done;
    }
    request.key.channel = channel;
    request.key.generation = 1;
    request.object = 77;
    request.site = site;
    request.warn_ms = 10;
    strcpy(request.operation, "ReadAs(Number)");
    if (dvm_broker_begin(broker, &request, 0, &key) != DVM_BROKER_OK ||
        dvm_broker_progress(broker, key, DVM_BROKER_NETWORK_IO, 0, "waiting for upstream", 1) != DVM_BROKER_OK ||
        dvm_broker_tick(broker, 10) != DVM_BROKER_OK ||
        dvm_broker_poll(broker, listener, &event, &dropped) != DVM_BROKER_OK || dropped ||
        dvm_broker_encode(&event, frame, sizeof(frame), &size) != DVM_BROKER_OK)
    {
        goto done;
    }
    ok = fwrite(frame, 1, size, stdout) == size && !fflush(stdout);
    if (dvm_broker_finish(broker, key, 0, "", 11) != DVM_BROKER_OK || dvm_broker_forget(broker, key) != DVM_BROKER_OK)
    {
        ok = 0;
    }
done:
    dvm_broker_destroy(broker);
    return ok ? 0 : 1;
}
