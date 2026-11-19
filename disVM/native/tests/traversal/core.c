/* Copyright (c) 2026 bhgv
 * EN: Active-path traversal identity, parallel isolation, balanced growth and scalar wire rollback.
 * RU: Active-path identity обхода, parallel isolation, balanced growth и scalar wire rollback.
 */
#include "walk.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static WalkEntity entity; /* Immutable copied fixture projection identity after setup. */
static WalkScope held; /* Main-thread scope passed only to verify wrong-thread rejection. */


/* EN/RU: Explicit contract failure in every build; valid/why borrowed through return, no assert.
 * Явный отказ contract в любой сборке; valid/why borrowed до return, без assert.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "walk core: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Enter the same state while main holds it; native thread isolation forbids shared flags or foreign scope pops.
 * Войти в тот же state при held main; native thread isolation исключает общие flags и foreign scope pops.
 */
static void *parallel(void *unused)
{
    (void)unused;
    WalkScope scope = {0};
    check(walk_enter(&scope, &entity, 0, 1, WALK_INFO, "file", "") == WALK_OK &&
        !walk_leave(&held) && walk_depth() == 1 && walk_leave(&scope) && !walk_depth(), "parallel search falsely cycled");
    return NULL;
}


/* EN/RU: Exercise exact active states; same VM/root with different node/path/mode/operation is not automatically a cycle.
 * Проверить exact active states; другая node/path/mode/operation той же VM/root не является циклом сама по себе.
 */
static void states(void)
{
    WalkScope nested = {0};
    check(walk_enter(&held, &entity, 0, 1, WALK_INFO, "file", "") == WALK_OK, "root enter");
    check(walk_enter(&nested, &entity, 0, 1, WALK_INFO, "file", "") == WALK_CYCLE && !nested.active,
        "active semantic state repeated without cycle detection");
    check(walk_enter(&nested, &entity, 0, 2, WALK_INFO, "file", "") == WALK_OK && walk_leave(&nested), "different node refused");
    check(walk_enter(&nested, &entity, 0, 1, WALK_INFO, "other", "") == WALK_OK && walk_leave(&nested), "different remainder refused");
    check(walk_enter(&nested, &entity, 0, 1, WALK_ACCESS, "file", "r") == WALK_OK && walk_leave(&nested), "different operation refused");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, parallel, NULL) && !pthread_join(worker, NULL), "parallel worker");
    check(walk_depth() == 1 && walk_leave(&held) && !walk_depth(), "scope balance");
    check(walk_enter(&held, &entity, 0, 1, WALK_INFO, "file", "") == WALK_OK && walk_leave(&held),
        "finished union branch poisoned later traversal");
}


/* EN/RU: Copy a real active search, reject duplicates/truncation atomically, preserve independent remote history after buffer reuse.
 * Скопировать active search, атомарно отказать duplicate/truncated history, сохранить copied remote history после reuse bytes.
 */
static void snapshots(void)
{
    unsigned char bytes[4096];
    size_t size = 999;
    check(walk_enter(&held, &entity, 0, 1, WALK_INFO, "file", "") == WALK_OK && walk_export(bytes, sizeof(bytes), &size),
        "export active history");
    WalkScope imported = {0};
    check(walk_import(&imported, bytes, size) == WALK_CYCLE && !imported.active && walk_depth() == 1,
        "remote repeated history was not rolled back");
    check(walk_leave(&held), "leave original exported history");
    for (size_t prefix = 0; prefix < size; prefix++)
    {
        check(walk_import(&imported, bytes, prefix) < WALK_OK && !imported.active && !walk_depth(),
            "truncated history changed active search");
    }
    check(walk_import(&imported, bytes, size) == WALK_OK && walk_depth() == 1, "import copied remote history");
    memset(bytes, 0, size);
    WalkScope repeat = {0};
    check(walk_enter(&repeat, &entity, 0, 1, WALK_INFO, "file", "") == WALK_CYCLE && walk_leave(&imported) && !walk_depth(),
        "remote history borrowed overwritten receive bytes");
    check(walk_export(bytes, sizeof(bytes), &size) && size == 4 &&
        walk_import(&imported, bytes, size) == WALK_OK && walk_leave(&imported), "empty remote scope");
}


/* EN/RU: Grow beyond inline storage, distinguish finite depth exhaustion from cycles, then completely unwind.
 * Вырасти за inline storage, отличить depth exhaustion от cycles, затем полностью unwind.
 */
static void deep(void)
{
    WalkScope scopes[WALK_DEPTH_LIMIT] = {{0}};
    for (unsigned i = 0; i < WALK_DEPTH_LIMIT; i++)
    {
        check(walk_enter(&scopes[i], &entity, 0, i, WALK_INFO, "file", "") == WALK_OK, "deep active stack growth");
    }
    WalkScope excess = {0};
    check(walk_enter(&excess, &entity, 1, 0, WALK_INFO, "file", "") == WALK_LIMIT && !excess.active,
        "depth exhaustion confused with a proven cycle");
    check(!walk_leave(&scopes[0]), "out-of-order scope leave corrupted stack");
    for (unsigned i = WALK_DEPTH_LIMIT; i; i--)
    {
        check(walk_leave(&scopes[i - 1]), "deep scope leave");
    }
    check(!walk_depth(), "deep storage did not retire");
}


/* EN/RU: Configure descriptive VM identity and exercise independent local/portable active histories; return0 on success.
 * Задать descriptive VM identity и проверить local/portable active histories; return0 при успехе.
 */
int main(void)
{
    unsigned long domain[4] = {1, 2, 3, 4};
    check(walk_domain(domain) && walk_entity(&entity), "native domain/entity");
    domain[3]++;
    check(!walk_domain(domain), "domain changed after publication");
    states();
    snapshots();
    deep();
    puts("PASS request-local active history, parallel isolation, copied wire state and depth cleanup");
    return 0;
}
