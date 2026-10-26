/* Real FlashVM channel acceptance: nonblocking backpressure, recovery and diagnostic-only traffic. */
#include "flash_channel.h"
#include "codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Keep assertions active in every build mode; this fixture has one serialized native owner. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL broker/FlashVM channel: %s\n", message);
        exit(1);
    }
}


/* There are no contending broker users in this fixture; production hosts supply a real mutex. */
static void serialized(void *unused)
{
    (void)unused;
}


/* Exercise actual copied-channel APIs; zero means every borrowed endpoint survives pump destruction. */
int main(void)
{
    DVM_BrokerSync sync = {serialized, serialized, NULL};
    DVM_Broker *broker = NULL;
    DVM_BrokerFlashPump *pump = NULL;
    FVM_Channel *output = NULL;
    FVM_Channel *rebound = NULL;
    FVM_Value value;
    DVM_BrokerSite source;
    DVM_BrokerChannel card;
    DVM_BrokerSnapshot input;
    DVM_BrokerEvent event;
    DVM_BrokerKey key;
    uint32_t origin;
    uint32_t channel;
    memset(&source, 0, sizeof(source));
    memset(&card, 0, sizeof(card));
    memset(&input, 0, sizeof(input));
    strcpy(source.build, "flash-channel-test");
    strcpy(source.path, "test.as");
    source.line = 1;
    source.column = 17;
    source.length = 1;
    require(dvm_broker_create(&sync, &broker) == DVM_BROKER_OK, "create");
    require(dvm_broker_site(broker, &source, &origin) == DVM_BROKER_OK, "source");
    card.transport = DVM_BROKER_IN_PROCESS;
    card.created_at = card.connected_at = origin;
    strcpy(card.logical, "other-vm");
    strcpy(card.peer, "owner:2");
    require(dvm_broker_open(broker, &card, 0, &channel) == DVM_BROKER_OK, "external in-process owner");
    require(fvm_channel_create(&output) == FVM_OK && fvm_channel_set_capacity(output, 1) == FVM_OK,
            "real bounded channel");
    require(fvm_channel_create(&rebound) == FVM_OK && fvm_channel_set_capacity(rebound, 1) == FVM_OK,
            "real recovery channel");
    require(dvm_broker_flash_create(broker, output, &pump) == FVM_OK, "global pump requires no channel/request IDs");
    require(dvm_broker_flash_step(pump) == FVM_NOT_FOUND, "no startup success diagnostic");
    input.key.channel = channel;
    input.key.generation = 1;
    input.site = origin;
    input.warn_ms = 1;
    strcpy(input.operation, "Get");
    require(dvm_broker_begin(broker, &input, 0, &key) == DVM_BROKER_OK, "admit");
    require(fvm_value_string(&value, "occupied") == FVM_OK && fvm_channel_try_send(output, &value) == FVM_OK,
            "fill actual transport");
    require(dvm_broker_tick(broker, 1) == DVM_BROKER_OK && dvm_broker_flash_step(pump) == FVM_NOT_FOUND,
            "full output never blocks or loses the pending deviation");
    require(fvm_channel_receive_owned(output, &value, 0) == FVM_OK, "drain occupied output");
    fvm_channel_message_release(&value);
    fvm_channel_close(output);
    require(dvm_broker_flash_step(pump) == FVM_ERROR, "closed output retains pending frame");
    require(dvm_broker_flash_rebind(pump, rebound) == FVM_OK && dvm_broker_flash_step(pump) == FVM_OK,
            "rebind delivers the same pending deviation");
    require(fvm_channel_receive_owned(rebound, &value, 0) == FVM_OK, "receive actual owned bytes");
    require(dvm_broker_decode(value.string, fvm_value_string_size(&value), &event) == DVM_BROKER_OK &&
                event.kind == DVM_BROKER_SLOW && event.origin.column == 17,
            "decode exact source from copied transport");
    fvm_channel_message_release(&value);
    require(dvm_broker_finish(broker, key, 0, "", 2) == DVM_BROKER_OK && dvm_broker_flash_step(pump) == FVM_NOT_FOUND,
            "successful completion generates no channel record");
    require(!dvm_broker_flash_dropped(pump), "no silent backpressure drops");
    dvm_broker_flash_destroy(pump);
    require(fvm_value_string(&value, "still borrowed") == FVM_OK && fvm_channel_try_send(rebound, &value) == FVM_OK,
            "destroying pump does not close caller's output");
    fvm_channel_destroy(output);
    fvm_channel_destroy(rebound);
    dvm_broker_destroy(broker);
    puts(
        "PASS real FlashVM diagnostic channel: global subscription, copy isolation, backpressure, rebind and success silence");
    return 0;
}
