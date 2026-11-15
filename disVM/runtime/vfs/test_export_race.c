/* Copyright (c) 2026 bhgv
 * EN: Native cancellation/grant and source-push/view-retirement races.
 * RU: Native races отмены/выдачи и source-push/освобождения view.
 */
#include "export.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Shared immutable inputs; barrier starts competing native operations.
 * Immutable inputs; barrier запускает competing native operations. */
typedef struct Race
{
    pthread_barrier_t start; /* Native start gate, never a guest pause. */
    DVM_Publication *publication; /* Borrowed until both competing calls join. */
    DVM_Provider *source; /* Retained throughout worker lifetime. */
    uint64_t id; /* Owner review identity, not a requester capability. */
} Race;


/* EN/RU: Fail acceptance explicitly without eliding any tested operation.
 * Явно завершить acceptance при отказе операции. */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr, "export race: %s\n", message);
        abort();
    }
}


/* EN/RU: Race one owner reply against cancellation; either result is permitted.
 * Выполнить owner reply одновременно с cancellation; оба исхода допустимы. */
static void *reply(void *state)
{
    Race *race = state;
    pthread_barrier_wait(&race->start);
    dvm_publication_reply(race->publication, race->id, race->source);
    return NULL;
}


/* EN/RU: Emit valid monotonic source invalidations while projected views retire.
 * Выдать monotonic source invalidations одновременно с retirement view. */
static void *emit(void *state)
{
    Race *race = state;
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    require(dvm_changes_pending(dvm_provider_changes(race->source), &cursor, DVM_CHANGE_ALL, &batch) == 1,
            "source session");
    pthread_barrier_wait(&race->start);
    for (unsigned index = 0; index < 500; index++)
    {
        DVM_ChangeEvent event = {batch.event.session, batch.event.generation + index + 1, DVM_CHANGE_RIGHTS, 0, 0};
        dvm_changes_notify(dvm_provider_changes(race->source), &event);
    }
    return NULL;
}


/* EN/RU: Exercise public capability lifetime, queue bounds, and automatic push.
 * Проверить public capability lifetime, queue bounds и automatic push. */
void test_export_races(void)
{
    DVM_Provider *source = dvm_view_freeze(dvm_view_create(NULL, NULL, NULL));
    DVM_ExportOptions options = {2, DVM_EXPORT_PREPARE, NULL, NULL, NULL, NULL, NULL};
    DVM_Publication *publication = dvm_publication_create(&options);
    DVM_ExportRequest request = {0};
    request.access = FVM_ACCESS_READ;
    request.peer[0] = 'p';
    request.name[0] = 'n';
    DVM_ExportTicket *first = dvm_publication_request(publication, &request);
    DVM_ExportTicket *second = dvm_publication_request(publication, &request);
    require(source && publication && first && second && !dvm_publication_request(publication, &request),
            "outstanding queue bound");
    DVM_ExportRequest review;
    require(dvm_publication_next(publication, &review) && review.id == 1 &&
            dvm_publication_next(publication, &review) && review.id == 2 &&
            !dvm_publication_next(publication, &review), "one FIFO request per next");
    dvm_export_release(first);
    dvm_export_release(second);
    Race race;
    race.publication = publication;
    race.source = source;
    require(!pthread_barrier_init(&race.start, NULL, 2), "barrier init");
    for (unsigned index = 0; index < 100; index++)
    {
        pthread_t worker;
        DVM_ExportTicket *ticket = dvm_publication_request(publication, &request);
        require(ticket && dvm_publication_next(publication, &review), "review after cancellation");
        race.id = review.id;
        require(!pthread_create(&worker, NULL, reply, &race), "reply worker");
        pthread_barrier_wait(&race.start);
        dvm_export_release(ticket);
        require(!pthread_join(worker, NULL) && !dvm_publication_reply(publication, race.id, source),
                "late reply resurrected cancelled capability");
    }
    pthread_t worker;
    require(!pthread_create(&worker, NULL, emit, &race), "push worker");
    pthread_barrier_wait(&race.start);
    for (unsigned index = 0; index < 100; index++)
    {
        DVM_View *builder = dvm_view_create(NULL, NULL, NULL);
        require(builder && dvm_view_map(builder, "/", source, ".", 4, DVM_VIEW_TREE), "race view mapping");
        DVM_Provider *view = dvm_view_freeze(builder);
        require(view != NULL, "race view freeze");
        dvm_provider_release(view);
    }
    require(!pthread_join(worker, NULL), "push join");
    pthread_barrier_destroy(&race.start);
    dvm_publication_close(publication);
    dvm_provider_release(source);
}
