/* Standalone native-data example; not a Dis heap/module ABI. */
#ifndef EXAMPLE_NATIVE_WALKER_H
#define EXAMPLE_NATIVE_WALKER_H
#include "disvm/range_access.h"


/* Caller keeps every reachable node alive and requires cooperating writers
 * to use the same domain. Stack nodes in main remain live until all reads end. */
typedef struct ExampleNode
{
    struct ExampleNode *tail; /* Borrowed native link; NULL terminates this example. */
} ExampleNode;


/* domain/source/output borrow live native storage. Return ACQUIRED after a
 * complete count, BUSY on owner contention, REFUSED on setup/cycle/count failure.
 * Failed operations preserve output; always end the access before returning.
 * No allocation, managed roots/owners or provenance/lifetime grant is supplied. */
DVM_CellResult example_chain_length(DVM_CellDomain *domain, ExampleNode **source,
    unsigned long *output);
#endif
