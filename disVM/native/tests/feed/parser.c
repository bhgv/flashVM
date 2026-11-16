/* Copyright (c) 2026 bhgv
 * EN: Actual bounded feed decoder/cursor algorithm with fixture-only partial/batched records.
 * RU: Настоящий feed decoder/cursor algorithm с fixture-only partial/batched records.
 */
#include "../../vfs/feed/worker.c"
#include <stdio.h>


/* EN/RU: Always-active fixture invariant.
 * Постоянный fixture invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "remote event parser: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Two-byte fixture codec exercises actual state machine; not a production wire schema.
 * Двухбайтный fixture codec проверяет state machine, не production wire schema. */
static int fixture(void *state, const void *bytes, size_t size, StyxEvent *event, size_t *consumed)
{
    (void)state;
    if (size < 2)
    {
        return 0;
    }
    const char *text = bytes;
    *event = (StyxEvent){10, 100, STYX_EVENT_CONTENT, STYX_EVENT_RESOURCE, 42};
    *consumed = 2;
    switch (text[0])
    {
        case 'g':
            event->generation = 105;
            break;
        case 'e':
            event->session = 11;
            event->generation = 1;
            break;
        case 'o':
            event->session = 9;
            event->generation = 999;
            break;
        case 't':
            event->generation = 101;
            break;
        case 'r':
            event->generation = 102;
            event->kinds = STYX_EVENT_RIGHTS;
            break;
        case 'x':
            *consumed = 0;
            break;
        case 'b':
            *consumed = size+1;
            break;
        case 'n':
            *consumed = 1;
            return 0;
        case 'p':
            *consumed = 0;
            return 0;
        case 'u':
            event->kinds = 128;
            break;
        case 'm':
            event->kinds = STYX_EVENT_NETWORK;
            break;
        case 'z':
            return -1;
        default:
            break;
    }
    return 1;
}


/* EN/RU: Apply fixture bytes to the real decoder, retaining partial input between calls.
 * Подать fixture bytes настоящему decoder с сохранением partial input. */
static int input(StyxFeed *feed, const char *text)
{
    size_t count = strlen(text);
    check(feed->used+count <= feed->config.capacity, "fixture capacity");
    memcpy(feed->bytes+feed->used, text, count);
    feed->used += count;
    return decode(feed);
}


/* EN/RU: Check partial/batch progress, remote epoch fences and independent local journal mapping.
 * Проверить partial/batch progress, remote epoch fences и independent local mapping. */
int main(void)
{
    StyxFeed feed = {0};
    check(!pthread_mutex_init(&feed.lock, NULL), "native mutex");
    feed.target = dvm_changes_create();
    check(feed.target != NULL, "target journal");
    feed.config.capacity = 128;
    feed.config.kinds = STYX_EVENT_RIGHTS|STYX_EVENT_CONTENT;
    feed.config.resources = 1;
    feed.config.decode = fixture;
    feed.bytes = malloc(feed.config.capacity);
    check(feed.bytes != NULL, "assembly buffer");
    check(input(&feed, "a") && feed.used == 1 && !feed.info.records, "partial record lost");
    check(input(&feed, ".") && !feed.used && feed.info.records == 1, "partial record completion");
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(feed.target, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == 2, "remote generation copied into local journal");
    check(input(&feed, "a.g.e.o.") && !feed.used && feed.info.records == 3 && feed.info.session == 11 && feed.info.generation == 1, "duplicate/gap/epoch fence");
    check(dvm_changes_pending(feed.target, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == 4, "batched events did not allocate local generations");
    const char *invalid[] = {"x.", "b.", "n.", "u.", "m.", "z."};
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); i++)
    {
        feed.used = 0;
        check(!input(&feed, invalid[i]) && feed.info.records == 3, "invalid decoder result accepted");
    }
    feed.used = 0;
    feed.config.capacity = 2;
    check(!input(&feed, "p."), "full buffer need-more did not fail");
    feed.config.capacity = 128;
    feed.used = 0;
    feed.info = (StyxFeedInfo){0};
    feed.config.kinds = STYX_EVENT_RIGHTS;
    check(input(&feed, "a.") && dvm_changes_pending(feed.target, &cursor, DVM_CHANGE_ALL, &batch) == 1, "new subscription snapshot");
    uint64_t before = batch.event.generation;
    check(input(&feed, "t.") && feed.info.records == 2 && feed.info.generation == 101, "unselected class cursor not advanced");
    check(dvm_changes_pending(feed.target, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == before, "unselected class published");
    check(dvm_changes_ack(feed.target, &cursor, &batch) == FVM_OK && input(&feed, "r."), "selected scoped record");
    check(dvm_changes_pending(feed.target, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.kinds == STYX_EVENT_RIGHTS &&
          batch.event.flags == STYX_EVENT_RESOURCE && batch.event.resource == 42, "filtered class produced false gap/scope loss");
    free(feed.bytes);
    dvm_changes_destroy(feed.target);
    pthread_mutex_destroy(&feed.lock);
    puts("PASS remote event partial/batch decoder and cursor fences");
    return 0;
}
