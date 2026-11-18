/* Copyright (c) 2026 bhgv
 * EN: Real Styx short reads on a native broker while consumers independently wait on per-tag queues.
 * RU: Настоящие короткие Styx-чтения на native-брокере и независимые ожидания потребителей по тегам.
 */
#include "check.h"
#include "client.h"
#include <pthread.h>


/* EN/RU: Own connected original client; fixture owns native physical peers until explicit join.
 * Получить подключённый исходный client; fixture владеет physical peers до join.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Join original server and release physical peers after all client users retire.
 * Сделать join исходного сервера и освободить physical peers после client users.
 */
extern void linkfixturefree(void);


/* EN/RU: Borrowed native broker arguments remain alive until join; neither field is guest-visible.
 * Аргументы native-брокера существуют до join; поля недоступны гостю.
 */
typedef struct Broker
{
    StyxOrder *order; /* Borrowed ordered-packet engine. */
    StyxFid *fid; /* Exclusive read fid, borrowed through join. */
} Broker;


/* EN/RU: Native broker alone consumes data lane; queues copies and never waits for a client to consume.
 * Только native-брокер читает data lane; копирует в очереди, не ждёт потребления клиентом.
 */
static void *receive(void *argument)
{
    Broker *broker = argument; /* Borrowed through join. */
    unsigned char storage[256]; /* Original S-stream retained body. */
    unsigned char packet_bytes[256]; /* Exact raw prefix/body retained until one complete feed. */
    FVM_SOrderedStream decoder; /* One native data-lane stream decoder. */
    size_t accumulated = 0; /* Raw bytes for current packet. */
    unsigned long long offset = 0; /* Original file stream offset. */
    unsigned records = 0; /* Exactly four fixture packets. */
    CHECK(fvm_sordered_stream_init(&decoder, storage, sizeof(storage), 128) == FVM_S_OK);
    while (records < 4)
    {
        unsigned char bytes[64]; /* One actual short original Styx read. */
        StyxTransfer result; /* Confirmed prefix; no guessed retry. */
        styxclientread(broker->fid, offset, bytes, sizeof(bytes), &result);
        CHECK(result.success && result.confirmed && result.count && result.count <= 3);
        offset += result.count;
        for (size_t at = 0; at < result.count;)
        {
            FVM_SOrderedRecord view; /* Borrowed verified packet, not yet consumed by client. */
            size_t used; /* Input consumed up to at most one packet. */
            FVM_SResult status = fvm_sordered_stream_push(&decoder, bytes + at, result.count - at, &view, &used); /* Exact parse result. */
            CHECK((status == FVM_S_OK || status == FVM_S_MORE) && used && accumulated + used <= sizeof(packet_bytes));
            memcpy(packet_bytes + accumulated, bytes + at, used);
            accumulated += used;
            at += used;
            if (status == FVM_S_OK)
            {
                CHECK(styxorderfeed(broker->order, packet_bytes, accumulated) == STYX_ORDER_ACCEPTED);
                records++;
                accumulated = 0;
                CHECK(fvm_sordered_stream_reset(&decoder) == FVM_S_OK);
            }
        }
    }
    return NULL;
}


/* EN/RU: Tag9 must be readable before releasing missing tag7 numbers through independent native Twrite.
 * Тег9 читается до разрешения пропущенных номеров тега7 отдельным native Twrite.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Owned real original Styx connection. */
    StyxOrder *order = styxordercreate(128, 8); /* Independent explicit ordered-data profile. */
    CHECK(client && order);
    StyxFid *input = styxclientwalk(client, "records"); /* Owned exclusive broker fid. */
    StyxFid *output = styxclientwalk(client, "records"); /* Owned independent command fid. */
    CHECK(input && output && styxclientopen(input, 0) && styxclientopen(output, 1));
    Broker broker = {order, input}; /* Borrowed by native worker through join. */
    pthread_t worker; /* Independent data-lane reader. */
    CHECK(!pthread_create(&worker, NULL, receive, &broker));
    CHECK(styxorderwait(order, 9, 3000));
    take(order, 9, 0, "nine");
    CHECK(!styxorderwait(order, 7, 0));
    unsigned char bytes[256]; /* Exact plain S-record service command. */
    size_t size; /* One complete encoded command. */
    StyxTransfer result; /* Exact Twrite acknowledgment. */
    CHECK(fvm_srecord_write(0, "release", 7, bytes, sizeof(bytes), &size) == FVM_S_OK);
    styxclientwrite(output, 0, bytes, size, &result);
    CHECK(result.success && result.confirmed && result.count == size);
    CHECK(styxorderwait(order, 7, 3000));
    take(order, 7, 0, "zero");
    CHECK(!pthread_join(worker, NULL));
    take(order, 7, 1, "one");
    take(order, 7, 2, "two");
    CHECK(offer(order, 7, 1, "one") == STYX_ORDER_DUPLICATE);
    styxclientclunk(input);
    styxclientclunk(output);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    styxorderfree(order);
    return 0;
}
