/* Continuous source/link exclusion, with explicit end after failed extension. */
#include "walker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Borrow domain/source/output and all reachable nodes under caller-proven life.
 * Capture source, add each link without dropping prior claims, then publish.
 * Brent checkpoints detect cycles without rereading unguarded links. Return
 * native status after end; no managed admission, retain/root or callback. */
DVM_CellResult example_chain_length(DVM_CellDomain *domain, ExampleNode **source,
    unsigned long *output)
{
    DVM_RangeAccess access = {0};
    DVM_CellResult result;
    ExampleNode *node;
    ExampleNode *checkpoint;
    unsigned long count = 0;
    result = dvm_range_access_three_begin(domain, source, sizeof(*source), source,
        sizeof(*source), output, sizeof(*output), &access);
    if (result != DVM_CELL_ACQUIRED)
    {
        return result;
    }
    memcpy(&node, source, sizeof(node));
    checkpoint = node;
    while (node)
    {
        if (count == (unsigned long)-1)
        {
            result = DVM_CELL_REFUSED;
            break;
        }
        result = dvm_range_access_extend(&access, &node->tail, sizeof(node->tail));
        if (result != DVM_CELL_ACQUIRED)
        {
            break;
        }
        count++;
        memcpy(&node, &node->tail, sizeof(node));
        if (node == checkpoint)
        {
            result = DVM_CELL_REFUSED;
            break;
        }
        if ((count & (count+1)) == 0)
        {
            checkpoint = node;
        }
    }
    if (result == DVM_CELL_ACQUIRED)
    {
        memcpy(output, &count, sizeof(count));
    }
    if (!dvm_range_access_end(&access))
    {
        fputs("native walker: access retirement failed\n", stderr);
        exit(1);
    }
    return result;
}
