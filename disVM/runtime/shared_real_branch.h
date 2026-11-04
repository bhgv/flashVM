/* Two-phase shared REAL inputs and lazy native branch target. */
#ifndef DVM_SHARED_REAL_BRANCH_H
#define DVM_SHARED_REAL_BRANCH_H


/* [GC:] condition0..5 selects EQ/NE/LT/LE/GT/GE; source/other borrow live REAL
 * cells, target a live native pointer cell only if taken; pc is exact TLS PC.
 * Return static error/NULL after unlock; native FP faults may unwind between
 * phases with no guards held. Pair snapshot is coherent; target is sampled later,
 * separately, never promised atomic with inputs. Caller owns mapping/frame/code
 * lifetimes and collector exclusion. No allocation, retain or root is introduced.
 * Native metadata mutexes retain their existing wait policy. */
const char *dvm_runtime_real_branch(int condition, const void *source,
    const void *other, const void *target, void *pc);
#endif
