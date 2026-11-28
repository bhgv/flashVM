/* Copyright (c) 2026 bhgv
 * EN: Serve a copied graph with multiple leased queues plus two owner-operated mailboxes.
 * RU: Обслуживание графа с несколькими арендованными очередями и двумя очередями владельца.
 */
#include "styxrpc/module.h"
#include "styxrpc/graph.h"
#include "styxrpc/channel_mailbox.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>

#define EXAMPLE_FRAME 192U                    /* Bound for the small tuples and updates in this example. */
#define COMPOSE_SIG UINT32_C(0xba190857)      /* Limbo compiler signature for Results.compose. */
#define BUFFERED_SIG UINT32_C(0x57a0e232)     /* Limbo compiler signature for Results.buffered. */
#define MEETING_SIG UINT32_C(0x09ef996d)      /* Limbo compiler signature for Results.meeting. */
#define NUMBER_KEY 1U                         /* First integer queue of the composite result. */
#define TEXT_KEY 2U                           /* Second queue, which holds strings. */

static const StyxRpcSchema compound_schema = {"(r[i]is)", "((r[i]a[c[i]]c[s]))"}; /* Native offer shape. */


typedef struct Owner
{
    StyxRpcChannelMailbox *text; /* Owned handle for a one-slot duplex result. */
    StyxRpcChannelMailbox *sync; /* Owned handle for the result with zero slots. */
    int64_t increment;           /* Copied argument for the integer reply. */
    int64_t pending;             /* Integer reply waiting for a peer receive. */
    int received;                /* The peer's integer has already been consumed once. */
    int sent;                    /* The owner confirmed the integer reply. */
    int checked;                 /* The guest observed both resources retire. */
} Owner;


/* EN: Append one complete record to a small example buffer.
 * RU: Добавить полную запись в ограниченный буфер примера. */
static int append(unsigned long tag, const void *payload, size_t size, unsigned char *bytes, size_t *extent)
{
    size_t used;
    if (fvm_srecord_write(tag, payload, size, bytes + *extent, EXAMPLE_FRAME - *extent, &used) != FVM_S_OK)
    {
        return 0;
    }
    *extent += used;
    return 1;
}


/* EN: Append either a checked local key or nil at one typed result position.
 * RU: Добавить проверенный локальный ключ либо nil в типизированную позицию. */
static int port(const char *shape, uint32_t key, unsigned char *bytes, size_t *extent)
{
    if (!key)
    {
        return append(STYXRPC_VALUE_NIL, NULL, 0, bytes, extent);
    }
    StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT};
    size_t used;
    route.channel.key = key;
    strcpy(route.channel.shape, shape);
    if (styxrpc_channel_route_value(&route, bytes + *extent, EXAMPLE_FRAME - *extent, &used) != STYXRPC_OK)
    {
        return 0;
    }
    *extent += used;
    return 1;
}


/* EN: Encode a counted string or nil inside the existing typed channel element frame.
 * RU: Закодировать строку с длиной или nil в штатном кадре элемента канала. */
static int string_frame(const FVM_SRecord *text, unsigned char *bytes, size_t *size)
{
    unsigned char child[EXAMPLE_FRAME];
    size_t extent = 0;
    *size = 0;
    return append(text->tag, text->payload, text->size, child, &extent) &&
        append(STYXRPC_CHANNEL_ELEMENT_TAG, child, extent, bytes, size);
}


/* EN: Seed two queues and copy them into one owned resource before local buffers expire.
 * RU: Заполнить две очереди и скопировать их в общий ресурс до освобождения локальных буферов. */
static int channels(StyxRpcChannelOutput *output)
{
    unsigned char number[EXAMPLE_FRAME], text[EXAMPLE_FRAME];
    size_t number_size, text_size;
    const FVM_SRecord label = {.tag = STYXRPC_VALUE_STRING, .payload = (const unsigned char *)"label", .size = 5};
    if (styxrpc_value_integer(STYXRPC_VALUE_INT, 71, number, sizeof(number), &number_size) != STYXRPC_OK ||
        !string_frame(&label, text, &text_size))
    {
        return 0;
    }
    const StyxRpcChannelQueue seeds[] = {
        {.grant = {NUMBER_KEY, "(i)"}, .bytes = number, .sizes = &number_size, .capacity = 1,
            .stride = sizeof(number), .count = 1},
        {.grant = {TEXT_KEY, "(s)"}, .bytes = text, .sizes = &text_size, .capacity = 1,
            .stride = sizeof(text), .count = 1}
    };
    return styxrpc_channel_group_output(seeds, sizeof(seeds) / sizeof(*seeds), 0, output) == STYXRPC_OK;
}


/* EN: Encode the original ref alias, repeated integer port and separate string position.
 * RU: Кодировать исходную ссылку, повторный канал int и отдельную строковую позицию. */
static int result_value(unsigned char id, int64_t option, StyxRpcChannelOutput *output)
{
    unsigned char list[EXAMPLE_FRAME], tuple[EXAMPLE_FRAME];
    size_t list_size = 0, tuple_size = 0;
    int live = option != 2;
    output->result = malloc(EXAMPLE_FRAME);
    return output->result && append(STYXRPC_VALUE_REF, &id, 1, tuple, &tuple_size) &&
        (!live || (port("(i)", NUMBER_KEY, list, &list_size) && port("(i)", NUMBER_KEY, list, &list_size))) &&
        port("(i)", 0, list, &list_size) &&
        append(STYXRPC_VALUE_ARRAY, list, list_size, tuple, &tuple_size) &&
        port("(s)", live ? (option == 1 ? NUMBER_KEY : TEXT_KEY) : 0, tuple, &tuple_size) &&
        append(STYXRPC_VALUE_TUPLE, tuple, tuple_size, output->result, &output->result_size);
}


/* EN: Change a checked ref and transfer the result child and sparse update to the dispatcher.
 * RU: Изменить проверенный ref и передать потомка результата и разреженное изменение диспетчеру. */
static StyxRpcStatus compose(void *unused, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    FVM_SRecord before, setting, absent;
    unsigned char fields[EXAMPLE_FRAME], scratch[EXAMPLE_FRAME];
    size_t used, length, tail, new_size;
    int64_t number, option;
    (void)unused;
    if (fvm_srecord_read(call->arguments.payload, call->arguments.size, EXAMPLE_FRAME, &before, &used) != FVM_S_OK ||
        before.tag != STYXRPC_VALUE_REF || before.size < 2 ||
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, before.payload + 1, before.size - 1, &number) != STYXRPC_OK ||
        number > INT32_MAX - 5 ||
        fvm_srecord_read(call->arguments.payload + used, call->arguments.size - used,
            EXAMPLE_FRAME, &setting, &length) != FVM_S_OK ||
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, call->arguments.payload + used, length, &option) != STYXRPC_OK ||
        option < 0 || option > 2 ||
        fvm_srecord_read(call->arguments.payload + used + length, call->arguments.size - used - length,
            EXAMPLE_FRAME, &absent, &tail) != FVM_S_OK ||
        absent.tag != STYXRPC_VALUE_NIL || absent.size || used + length + tail != call->arguments.size)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    if (option != 2 && !channels(output))
    {
        return STYXRPC_REMOTE_ERROR;
    }
    fields[0] = before.payload[0];
    output->updates = malloc(EXAMPLE_FRAME);
    if (!output->updates || styxrpc_value_integer(STYXRPC_VALUE_INT, number + 5,
        fields + 1, sizeof(fields) - 1, &new_size) != STYXRPC_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    FVM_SRecord after = {.tag = STYXRPC_VALUE_REF, .payload = fields, .size = new_size + 1};
    return styxrpc_graph_diff(&compound_schema, &call->arguments, fields[0], "r[i]", &before, &after,
        scratch, sizeof(scratch), output->updates, EXAMPLE_FRAME, &output->update_size) &&
        result_value(fields[0], option, output) ? STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* EN: Give the remote caller a buffered port; retain the independent owner handle here.
 * RU: Выдать вызывающему буферизованный конец, сохранив независимую ссылку владельца. */
static StyxRpcStatus buffered(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    Owner *owner = context;
    FVM_SRecord seed;
    unsigned char frame[EXAMPLE_FRAME];
    size_t used, extent;
    if (owner->text || fvm_srecord_read(call->arguments.payload, call->arguments.size,
        EXAMPLE_FRAME, &seed, &used) != FVM_S_OK || used != call->arguments.size ||
        !string_frame(&seed, frame, &extent) ||
        styxrpc_channel_mailbox_result_create("(s)", 1, &owner->text, output) != STYXRPC_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    return styxrpc_channel_mailbox_send(owner->text, frame, extent);
}


/* EN: Return an unbuffered int port without posting any operation yet.
 * RU: Выдать синхронный конец int без предварительной операции. */
static StyxRpcStatus meeting(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    Owner *owner = context;
    if (owner->sync ||
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, call->arguments.payload, call->arguments.size,
            &owner->increment) != STYXRPC_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    return styxrpc_channel_mailbox_result_create("(i)", 0, &owner->sync, output);
}


/* EN: Expose result-fid retirement without consuming either endpoint.
 * RU: Сообщить об освобождении fid результатов без закрытия концов владельца. */
static int retired(void *context, int64_t ignored, int64_t *value)
{
    Owner *owner = context;
    (void)ignored;
    owner->checked = owner->text && owner->sync && owner->sent &&
        !styxrpc_channel_mailbox_active(owner->text) && !styxrpc_channel_mailbox_active(owner->sync);
    *value = owner->checked;
    return 1;
}


/* EN: Dequeue the expected incoming text and post an independent outgoing reply.
 * RU: Извлечь ожидаемый текст и поставить независимый исходящий ответ. */
static void echo_text(Owner *owner)
{
    unsigned char frame[EXAMPLE_FRAME];
    FVM_SRecord outer, child;
    size_t extent, used;
    if (styxrpc_channel_mailbox_receive(owner->text, frame, sizeof(frame), &extent) != STYXRPC_OK ||
        fvm_srecord_read(frame, extent, sizeof(frame), &outer, &used) != FVM_S_OK ||
        fvm_srecord_read(outer.payload, outer.size, sizeof(frame), &child, &used) != FVM_S_OK ||
        child.tag != STYXRPC_VALUE_STRING || child.size != 8 || memcmp(child.payload, "from-dis", 8))
    {
        return;
    }
    const FVM_SRecord echo = {.tag = STYXRPC_VALUE_STRING, .payload = (const unsigned char *)"echo:from-dis", .size = 13};
    if (string_frame(&echo, frame, &extent))
    {
        styxrpc_channel_mailbox_send(owner->text, frame, extent);
    }
}


/* EN: Match one waiting peer sender and remember the reply until a peer receiver arrives.
 * RU: Сопоставить ожидающего отправителя и хранить ответ до появления получателя. */
static void echo_int(Owner *owner)
{
    unsigned char frame[STYXRPC_MANAGED_LIMIT];
    size_t used;
    int64_t value;
    if (!owner->received && styxrpc_channel_mailbox_receive(owner->sync, frame, sizeof(frame), &used) == STYXRPC_OK &&
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, frame, used, &value) == STYXRPC_OK)
    {
        owner->received = 1;
        owner->pending = value + owner->increment;
    }
    if (owner->received && styxrpc_value_integer(STYXRPC_VALUE_INT, owner->pending,
        frame, sizeof(frame), &used) == STYXRPC_OK)
    {
        owner->sent = styxrpc_channel_mailbox_send(owner->sync, frame, used) == STYXRPC_OK;
    }
}


/* EN: Fill a private lease token from the native entropy source.
 * RU: Заполнить закрытый токен аренды из системного источника случайности. */
static int entropy(void *unused, void *bytes, size_t size)
{
    (void)unused;
    while (size)
    {
        ssize_t used = getrandom(bytes, size, 0);
        if (used < 0 && errno == EINTR)
        {
            continue;
        }
        if (used <= 0)
        {
            return 0;
        }
        bytes = (unsigned char *)bytes + used;
        size -= (size_t)used;
    }
    return 1;
}


/* EN: Serve an already connected carrier and run local mailbox work between wire requests.
 * RU: Обслуживать готовое соединение и работать с очередями между сетевыми запросами. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    Owner owner = {0};
    StyxRpcModuleChannels registry;
    StyxRpcModuleChannel leases[STYXRPC_MAX_EXPORTS];
    StyxRpcModule *module = NULL;
    const StyxRpcModuleEntry exports[] = {
        {.kind = STYXRPC_MODULE_CHANNEL, .callback.channel = {"compose", COMPOSE_SIG, compound_schema, compose, NULL}},
        {.kind = STYXRPC_MODULE_CHANNEL, .callback.channel = {"buffered", BUFFERED_SIG, {"(s)", "(c[s])"}, buffered, &owner}},
        {.kind = STYXRPC_MODULE_CHANNEL, .callback.channel = {"meeting", MEETING_SIG, {"(i)", "(c[i])"}, meeting, &owner}},
        {.kind = STYXRPC_MODULE_INT, .callback.integer = {.name = "retired", .signature = STYXRPC_INT_SIGNATURE,
            .invoke = retired, .context = &owner}}
    };
    const StyxRpcModuleSpec spec = {.name = "channel-results.dis", .entries = exports,
        .count = sizeof(exports) / sizeof(*exports), .registry = &registry};
    if (fd < 3 || fd > INT_MAX || *end ||
        !styxrpc_module_channels_init(&registry, leases, STYXRPC_MAX_EXPORTS, entropy, NULL) ||
        styxrpc_module_create(&spec, &module) != STYXRPC_OK)
    {
        return 2;
    }
    PhyBidir *carrier = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), STYXRPC_MANAGED_LIMIT);
    StyxRpcServer *server = carrier ? styxrpc_server_create(phybidirchannel(carrier, 0),
        &styxrpc_module_tree, module) : NULL;
    while (server && styxrpc_server_step_for(server, 2))
    {
        if (styxrpc_channel_mailbox_active(owner.text))
        {
            echo_text(&owner);
        }
        if (!owner.sent && styxrpc_channel_mailbox_active(owner.sync))
        {
            echo_int(&owner);
        }
    }
    styxrpc_server_destroy(server);
    styxrpc_channel_mailbox_owner_release(owner.sync);
    styxrpc_channel_mailbox_owner_release(owner.text);
    styxrpc_module_destroy(module);
    phybidirfree(carrier);
    if (owner.checked)
    {
        puts("PASS independent C result provider");
    }
    return owner.checked ? 0 : 1;
}
