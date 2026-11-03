/* Query point-of-use proofs independently of optimizer rewrites and VM state. */
#include "fir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef FIR_LOCALITY_THREADS
#include <pthread.h>
#endif


/* Shared verified graph is immutable; every worker owns separate fact storage. */
typedef struct Work
{
    const FirModule *module; /* Borrowed through joins, never parsed or mutated concurrently. */
    const FirLocalityFacts *expected; /* Borrowed immutable baseline. */
    int ok; /* Written by worker, observed only after join. */
} Work;


/* Resolve one definition by printable name; module/name borrowed. Return its
 * instruction/value index through outputs, or0 on absence. No mutation. */
static int find(const FirModule *module, const char *name, int *instruction, int *value)
{
    int index;
    for (index = 0; index < module->value_count; index++)
    {
        if (!strcmp(module->values[index].name, name))
        {
            *value = index;
            *instruction = module->values[index].definition;
            return 1;
        }
    }
    return 0;
}


/* Repeat pure analysis with private scratch. argument borrows stable Work until
 * join; return NULL. No parser/global token state or runtime heap is accessed. */
static void *analyse(void *argument)
{
    Work *work = argument;
    FirLocalityFacts *facts = malloc(sizeof(*facts));
    int repeat;
    work->ok = facts != NULL;
    for (repeat = 0; facts && repeat < 100; repeat++)
    {
        fir_locality_facts(work->module, facts);
        if (memcmp(facts, work->expected, sizeof(*facts)))
        {
            work->ok = 0;
            break;
        }
    }
    free(facts);
    return NULL;
}


/* Parse/verify one graph and check mutable-cell snapshots vs referent ownership.
 * No parameters; return0 only for complete success. Inputs/graphs/facts/threads
 * are owned here; all cleanup paths join started workers before freeing inputs. */
int main(void)
{
    static const char source[] =
        "FIR 1.0\nmodule @locality\nfunc @main() -> void {\n^entry:\n"
        "%a: table.big = table.new\n%b: table.big = table.new\n"
        "%cell: table.cell = ref.local %a\n%alias: table.big = ref.load %cell\n"
        "%key: i64 = const.i64 0\n%neg: i64 = const.i64 -1\n"
        "%slot: table.slot = table.index %alias, %key\n%copy: table.slot = copy %slot\n"
        "%forty: i64 = const.i64 40\ntable.store %copy, %forty\ntable.lock %alias\n"
        "ref.store %cell, %b\n%other: table.big = ref.load %cell\n"
        "%other_slot: table.slot = table.index %other, %key\n"
        "%first: i64 = table.load %copy\ntrace %first\n"
        "%after: i64 = table.load %slot\ntable.unlock %alias\n"
        "jump ^next\n^next:\n%cross: i64 = table.load %slot\nreturn\n}\n";
    FirModule *module = calloc(1, sizeof(*module));
    FirModule *saved = malloc(sizeof(*saved));
    FirLocalityFacts *facts = malloc(sizeof(*facts));
    FILE *input = fir_temporary();
    char error[256] = "";
    Work work[8] = {{0}};
    int started = 0;
    int definition;
    int value;
    int first;
    int a;
    int b;
    int index;
    int ok = 0;
#ifdef FIR_LOCALITY_THREADS
    pthread_t threads[8]; /* Each thread owns analysis scratch; graph remains shared read-only. */
#endif
    if (!module || !saved || !facts || !input || fputs(source, input) == EOF ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    memcpy(saved, module, sizeof(*module));
    fir_locality_facts(module, facts);
    if (memcmp(saved, module, sizeof(*module)) ||
        !find(module, "%a", &definition, &a) || !find(module, "%b", &definition, &b) ||
        !find(module, "%first", &first, &value) || !facts->private_table[first] ||
        !facts->retained_table[first] || facts->table_owner[first] != a ||
        !facts->absolute_key[first] || facts->numeric_key[first] != 0 ||
        !find(module, "%other_slot", &definition, &value) || facts->table_owner[definition] != b ||
        !find(module, "%cell", &definition, &value) || !facts->frame_local[value] ||
        !find(module, "%after", &definition, &value) || facts->private_table[definition] ||
        !find(module, "%cross", &definition, &value) || facts->table_owner[definition] != -1 ||
        facts->private_table[definition] || facts->retained_table[definition])
    {
        fputs("FAIL locality snapshots, opaque barrier or CFG boundary\n", stderr);
        goto done;
    }
    for (index = 0; index < 8; index++)
    {
        work[index].module = module;
        work[index].expected = facts;
#ifdef FIR_LOCALITY_THREADS
        if (pthread_create(&threads[index], NULL, analyse, &work[index]))
        {
            goto join;
        }
#else
        (void)analyse(&work[index]);
#endif
        started++;
    }
#ifdef FIR_LOCALITY_THREADS
join:
    for (index = 0; index < started; index++)
    {
        if (pthread_join(threads[index], NULL))
        {
            work[index].ok = 0;
        }
    }
#endif
    ok = started == 8;
    for (index = 0; index < started; index++)
    {
        ok = ok && work[index].ok;
    }
    ok = ok && !memcmp(saved, module, sizeof(*module));
done:
    if (input)
    {
        fclose(input);
    }
    free(facts);
    free(saved);
    free(module);
    if (!ok)
    {
        fprintf(stderr, "locality API failed: %s\n", error);
        return 1;
    }
    puts("PASS locality API: immutable input, ref-cell snapshots, opaque/CFG invalidation, independent analysis scratch");
    return 0;
}
