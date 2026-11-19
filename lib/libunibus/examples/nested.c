/* Copyright (c) 2026 bhgv
 * EN: Hand-reviewed, allocation-free C89 tutorial: assign a service tag, receive
 * a fragmented envelope and decode the client's independent inner namespace.
 * RU: Вручную описанный C89 пример без allocation: назначить служебный тег,
 * принять фрагменты конверта и разобрать отдельное пространство тегов клиента.
 * standalone-reviewed: keep these teaching contracts instead of generating prose.
 */
#include "flashvm/sstream.h"   /* Incremental receiver; carrier fragments need not match record boundaries. */
#include "flashvm/sbindings.h" /* Broker-owned mapping from outer tags to agreed payload schemas. */
#include <stdio.h>             /* Failure diagnostics go to stderr, never into a binary carrier. */
#include <string.h>            /* Copy the fixed schema name and compare the received sensor bytes. */


/* EN: Report the failed tutorial stage, without allocating or retaining pointers.
 * stage is a borrowed NUL-terminated description; result is the codec status.
 * Returns 1 for process failure; synchronous stderr output may block.
 * RU: Вывести этап ошибки без allocation и удержания указателей.
 * stage — заимствованная C-строка; result — статус codec.
 * Вернуть 1 для ошибки процесса; синхронная запись stderr может блокироваться.
 */
static int failed(const char *stage, FVM_SResult result)
{
    if (result == FVM_S_OK) /* A valid frame can still violate this application's expected schema or extent. */
    {
        fprintf(stderr, "%s: valid codec result, unexpected content or extent\n", stage); /* Do not call OK a codec error. */
    }
    else
    {
        fprintf(stderr, "%s: codec status %d\n", stage, (int)result); /* Identify the operation and actual failure/continuation status. */
    }
    return 1; /* The caller has no heap, socket or thread to release before exiting. */
}


/* EN: Run the complete in-memory example; no arguments or external resources.
 * Returns 0 for the exact expected nested message, 1 for a reported failure.
 * All storage belongs to this invocation; consume borrowed views before reset.
 * RU: Выполнить пример в памяти; аргументов и внешних ресурсов нет.
 * Вернуть 0 для ожидаемого вложенного сообщения, 1 при диагностированной ошибке.
 * Все буферы принадлежат вызову; заимствованные views обработать до reset.
 */
int main(void)
{
    FVM_SBinding assignment; /* The service owner proposes one outer client/schema assignment. */
    FVM_SBinding slots[2];   /* Caller-owned table capacity: two assignments; this example needs one. */
    FVM_SBinding found;      /* Lookup copies the agreed assignment; it does not expose table storage. */
    FVM_SBindings bindings;  /* The application, not the sensor client, owns this transport-level table. */
    FVM_SStream stream;      /* One decoder instance for this receive direction. */
    FVM_SRecord outer;       /* On successful push, payload borrows receive_storage. */
    FVM_SRecord inner;       /* After inner decode, payload borrows the same receive_storage. */
    unsigned char metadata[128];        /* Encoded service payload, before its tag-zero envelope. */
    unsigned char wire[256];            /* A complete record produced by the simulated sender. */
    unsigned char nested[128];          /* The client's encoded inner record, before outer routing. */
    unsigned char receive_storage[256]; /* Distinct receiver storage: input/output must not overlap. */
    size_t metadata_size; /* Service payload bytes returned only after successful encoding. */
    size_t wire_size;     /* Complete transport record bytes to feed into the receiver. */
    size_t nested_size;   /* Complete inner record bytes, including its own framing. */
    size_t consumed;      /* Exact bytes accepted from the current fragment or parsed record. */
    size_t index;         /* Sender cursor; a real carrier can deliver larger or smaller chunks. */
    FVM_SResult result;   /* Every codec step is checked before its output is used. */

    memset(&assignment, 0, sizeof(assignment)); /* Initialize all fields, including reserved native storage. */
    assignment.tag = 7; /* Outer tag 7 routes this client; it is unrelated to inner application tag 1. */
    assignment.version = 1; /* This is payload schema version 1, not a sequence generation. */
    strcpy(assignment.type, "example.sensor.v1"); /* Fixed string fits the declared schema-name array. */
    assignment.type_size = sizeof("example.sensor.v1") - 1; /* Wire name length excludes its native NUL. */

    result = fvm_sbindings_init(&bindings, slots, 2); /* Bind the table to caller storage without allocation. */
    if (result != FVM_S_OK)
    {
        return failed("binding table initialization", result); /* No assignment may be used before init succeeds. */
    }
    result = fvm_sbindings_payload_write(&assignment, 1, metadata, sizeof(metadata), &metadata_size); /* Encode one schema assignment. */
    if (result != FVM_S_OK)
    {
        return failed("schema payload encoding", result); /* Do not wrap partially produced metadata. */
    }
    result = fvm_srecord_write(0, metadata, metadata_size, wire, sizeof(wire), &wire_size); /* Tag zero belongs to the connection service. */
    if (result != FVM_S_OK)
    {
        return failed("service envelope encoding", result); /* The recipient must receive a complete checked record. */
    }
    result = fvm_sbindings_apply(&bindings, wire, wire_size, &consumed); /* The service owner applies the received assignment. */
    if (result != FVM_S_OK || consumed != wire_size)
    {
        return failed("service assignment acceptance", result); /* This fixture expects exactly one complete service record. */
    }

    result = fvm_srecord_write(1, "21.5", 4, nested, sizeof(nested), &nested_size); /* Client-local tag 1 carries four sensor bytes. */
    if (result != FVM_S_OK)
    {
        return failed("sensor record encoding", result); /* Do not publish a partial inner record. */
    }
    result = fvm_srecord_write(7, nested, nested_size, wire, sizeof(wire), &wire_size); /* Broker wraps the client's complete record in outer tag 7. */
    if (result != FVM_S_OK)
    {
        return failed("client envelope encoding", result); /* Routing and the nested payload share one checked outer record. */
    }
    result = fvm_sstream_init(&stream, receive_storage, sizeof(receive_storage), 256); /* Local body quota; no OS or transport is opened. */
    if (result != FVM_S_OK)
    {
        return failed("incremental receiver initialization", result); /* Capacity/quota errors are local configuration failures. */
    }
    for (index = 0; index < wire_size; index++) /* Deliberately fragment every transport byte to demonstrate incremental reception. */
    {
        result = fvm_sstream_push(&stream, wire + index, 1, &outer, &consumed); /* The decoder retains consumed bytes internally. */
        if (consumed != 1 || result != (index + 1 == wire_size ? FVM_S_OK : FVM_S_MORE))
        {
            return failed("fragment acceptance", result); /* Only the last byte may publish this complete envelope. */
        }
    }
    result = fvm_sbindings_lookup(&bindings, outer.tag, &found); /* Check routing/schema before interpreting client bytes. */
    if (result != FVM_S_OK || outer.tag != 7 || found.version != 1 || strcmp(found.type, "example.sensor.v1"))
    {
        return failed("client schema lookup", result); /* A valid checksum does not imply the expected application schema. */
    }
    result = fvm_srecord_read(outer.payload, outer.size, 128, &inner, &consumed); /* Decode inside this client's namespace only. */
    if (result != FVM_S_OK || consumed != outer.size || inner.tag != 1 || inner.size != 4)
    {
        return failed("nested record decoding", result); /* Reject trailing data or a different inner message shape. */
    }
    if (memcmp(inner.payload, "21.5", 4)) /* Compare counted bytes; no NUL terminator is part of the sensor payload. */
    {
        return failed("sensor payload comparison", FVM_S_OK); /* Report a valid frame containing the wrong application data. */
    }
    result = fvm_sstream_reset(&stream); /* Consume the borrowed views before allowing their storage to be overwritten. */
    if (result != FVM_S_OK)
    {
        return failed("receiver reset", result); /* Reset is decoder reuse, not cancellation of remote work. */
    }
    puts("PASS tag0 binding, fragmented outer client7, inner application tag1 and exact payload"); /* Human-readable result, not carrier bytes. */
    return 0; /* Stack-owned storage is released after all borrowed views have been consumed. */
}
