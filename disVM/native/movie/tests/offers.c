/* Copyright (c) 2026 bhgv
 * EN: Native admission staging, concurrent replacement, directory projection and held-open identity.
 * RU: Native admission staging, concurrent replacement, directory projection и held-open identity.
 */
#include "offer.h"
#include "service.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Fail explicitly with a diagnostic; no release-build assertion side effects.
 * Завершить с diagnostic при нарушении contract, без assert. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "movie offers: %s\n", why);
        exit(2);
    }
}


typedef struct Race
{
    DVM_MovieOffer *source; /* Borrowed until joined. */
    DVM_Provider *provider; /* Borrowed immutable service through join. */
    uint64_t result; /* Written by worker, read after join. */
} Race;


/* EN/RU: Compete for exact generation1; no VM, transport or provider operation.
 * Конкурировать за exact generation1 без VM/transport/provider operations. */
static void *replace(void *state)
{
    Race *race = state;
    race->result = dvm_movie_offer_bind(race->source, 1, race->provider);
    return NULL;
}


/* EN/RU: Observe notices after source unlock; count exact synchronous registry transitions.
 * Наблюдать notices после source unlock; считать synchronous registry transitions. */
static void notice(void *state, const DVM_ChangeEvent *event)
{
    (void)event;
    __atomic_add_fetch((unsigned *)state, 1, __ATOMIC_RELAXED);
}


/* EN/RU: Open and consume one ordinary live directory listing; borrowed root/path.
 * Открыть/поглотить один live directory listing; root/path borrowed. */
static void listing(DVM_Provider *root, const char *path, size_t expected, const char *first)
{
    DVM_VfsHandle directory = {0};
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    check(dvm_provider_open(root, path, "r", &directory) == FVM_OK, "directory open");
    check(dvm_vfs_list(&directory, &entries, &count) == FVM_OK && count == expected,
        "live directory count");
    check(!first || (count && !strcmp(entries[0].name, first)), "live directory name");
    free(entries);
    dvm_vfs_close(&directory);
}


/* EN/RU: Exercise reservation rollback, exact scoped names, CAS and ordinary VFS lifetimes.
 * Проверить reservation rollback, exact scoped names, CAS и ordinary VFS lifetimes. */
int main(void)
{
    DVM_Namespace *group = dvm_namespace_create("/offer-no-native-fallback", "/", "/");
    DVM_Provider *service = group ? styxmovieservice(group) : NULL;
    DVM_MovieOffer *source = dvm_movie_offer_create();
    DVM_MovieOffer *aborted = dvm_movie_offer_create();
    DVM_Changes *journal = dvm_movie_offer_observe(source);
    unsigned notices = 0;
    DVM_NoticeTap *tap = dvm_notice_subscribe(dvm_changes_notices(journal), notice, &notices, NULL);
    check(service && source && aborted && journal && tap, "native setup");
    DVM_Provider *root = styxmovieofferroot(source, service);
    check(root != NULL, "ordinary directory projection");
    FVM_FileInfo metadata;
    check(dvm_provider_info(root, ".", &metadata) == FVM_OK && metadata.directory &&
        metadata.mode == 0555, "searchable read-only root");
    listing(root, ".", 2, "ctl");
    listing(root, "movie", 0, NULL);
    unsigned char bytes[MOVIE_OFFER_FRAME*2];
    size_t size = 999;
    check(styxmovieofferframe(source, 7, bytes, sizeof(bytes), &size) == FVM_NOT_FOUND && size == 999,
        "unowned source advertised");
    check(dvm_movie_offer_stage(source, service) && !dvm_movie_offer_stage(source, service),
        "exclusive reservation");
    check(!dvm_movie_offer_acquire(source, "movie/1") && notices == 0 &&
        styxmovieofferframe(source, 7, bytes, sizeof(bytes), &size) == FVM_NOT_FOUND,
        "staged service escaped");
    check(dvm_movie_offer_stage(aborted, service) && dvm_movie_offer_abort(aborted) &&
        !dvm_movie_offer_commit(aborted) && !dvm_movie_offer_bind(aborted, 0, service),
        "aborted reservation reused");
    DVM_MovieOfferInfo snapshot;
    check(dvm_movie_offer_info(aborted, &snapshot) && snapshot.closed && !snapshot.ready &&
        snapshot.generation == 1, "rollback state");
    check(dvm_movie_offer_commit(source) == 1 && !dvm_movie_offer_commit(source) &&
        !dvm_movie_offer_abort(source) && notices == 1, "one initial publication");
    check(styxmovieofferframe(source, 7, bytes, sizeof(bytes), &size) == FVM_OK && size,
        "ready wire announcement");
    check(!movie_offer_record(7, MOVIE_OFFER_READY, 0, bytes, sizeof(bytes), &size) &&
        !movie_offer_record(7, MOVIE_OFFER_READY, MOVIE_OFFER_GENERATION_MAX+1,
            bytes, sizeof(bytes), &size) &&
        movie_offer_record(7, MOVIE_OFFER_CLOSED, MOVIE_OFFER_GENERATION_MAX,
            bytes, sizeof(bytes), &size), "wire generation bounds");
    listing(root, "movie", 1, "1");
    DVM_Provider *pin = dvm_movie_offer_acquire(source, "movie/1");
    check(pin == service && !dvm_movie_offer_acquire(source, "movie/01") &&
        !dvm_movie_offer_acquire(aborted, "movie/1"), "canonical scoped admission");
    DVM_VfsHandle held = {0};
    check(dvm_provider_open(root, "movie/1", "rw", &held) == FVM_OK, "held old generation");
    Race races[2] = {{source, service, 0}, {source, service, 0}};
    pthread_t threads[2];
    check(!pthread_create(threads, NULL, replace, races) &&
        !pthread_create(threads+1, NULL, replace, races+1), "replacement threads");
    check(!pthread_join(threads[0], NULL) && !pthread_join(threads[1], NULL), "replacement joins");
    check((races[0].result == 2) != (races[1].result == 2), "CAS admitted two replacement owners");
    check(notices == 2 && !dvm_movie_offer_close(source, 1) &&
        !dvm_movie_offer_acquire(source, "movie/1"), "stale generation retained admission");
    listing(root, "movie", 1, "2");
    check(dvm_vfs_info(&held, &metadata) == FVM_OK && metadata.identity.low == 1 &&
        !strcmp(metadata.name, "1"), "held open identity changed");
    check(dvm_movie_offer_close(source, 2) && notices == 3 &&
        !dvm_movie_offer_bind(source, 3, service) && !dvm_movie_offer_acquire(source, "movie/2"),
        "closed source reopened");
    listing(root, "movie", 0, NULL);
    check(dvm_vfs_info(&held, &metadata) == FVM_OK, "retirement revoked held file lifetime");
    dvm_vfs_close(&held);
    dvm_provider_release(pin);
    dvm_provider_release(root);
    dvm_notice_unsubscribe(tap);
    dvm_changes_destroy(journal);
    dvm_movie_offer_release(source);
    dvm_movie_offer_release(aborted);
    dvm_provider_release(service);
    dvm_namespace_release(group);
    puts("PASS movie offer staging, CAS, scoped admission and held-open projection");
    return 0;
}
