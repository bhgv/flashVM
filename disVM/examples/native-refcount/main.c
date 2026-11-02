/* Native immutable-message handoff. This is not a mapped Dis heap object. */
#include "disvm/refcount.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>


/* Each owner holds one count; payload is immutable after thread publication. */
typedef struct Message
{
    unsigned long references; /* Access atomically after publication, while already held alive. */
    int value;                /* Native scalar copied before the worker can observe it. */
} Message;


/* Drop exactly one owned reference. message must remain live through the atomic
 * operation. Only the LAST owner frees it; no code touches it after this call. */
static void message_drop(Message *message)
{
    DVM_RefcountResult result = dvm_refcount_release_u(&message->references);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        fputs("invalid native message reference\n", stderr);
        exit(1);
    }
    if (result == DVM_REFCOUNT_LAST)
    {
        free(message);
    }
}


/* argument transfers one already-retained strong reference to this OS thread.
 * No raw-pointer resurrection or header lock is attempted. Payload is read-only,
 * and the worker drops its ownership after its final payload access. */
static void consume(void *argument)
{
    Message *message = argument;
    if (message->value != 42)
    {
        fputs("native handoff payload changed\n", stderr);
        exit(1);
    }
    message_drop(message);
}


/* Publish a retained handoff, release the sender independently and join. There
 * are no VM roots or cross-heap pointers. Native creation failure rolls back the
 * unpublished handoff reference while the sender still owns its initial one. */
int main(void)
{
    Message *message = calloc(1, sizeof(*message));
    FVM_PortThread *thread;
    if (!message)
    {
        return 1;
    }
    message->references = 1; /* Exclusive initialization before publication is permitted. */
    message->value = 42;
    if (dvm_refcount_retain_u(&message->references) != DVM_REFCOUNT_LIVE)
    {
        message_drop(message);
        return 1;
    }
    thread = fvm_port_thread_start(consume, message);
    if (!thread)
    {
        message_drop(message); /* Roll back the handoff that no worker received. */
        message_drop(message); /* Release the sender's remaining owner. */
        return 1;
    }
    message_drop(message); /* Sender must not inspect message after dropping it. */
    fvm_port_thread_join(thread);
    puts("PASS native message handoff: retained before publication, unique last owner frees");
    return 0;
}
