/* Copyright (c) 2026 bhgv
 * EN: Test-only omission of expected revision, preserving the real namespace reader.
 * RU: Только тестовый пропуск expected revision с настоящим namespace reader.
 */
#include "vfs/observation.h"

int __real_dvm_namespace_observe(
    DVM_Namespace *, const uint64_t *, size_t, int, size_t, unsigned char *, size_t, DVM_NamespaceObservation *);


/* EN: Replace caller revision with current summary, then perform real text read.
 * This deliberately accepts mixed pages; no production code or state is altered.
 * RU: Заменить caller revision текущей summary и реально прочитать text.
 * Это намеренно допускает mixed pages; production code/state не меняются.
 * Arguments/results retain observation.h ownership; only test linker uses this. */
int __wrap_dvm_namespace_observe(DVM_Namespace *group,
                                 const uint64_t *expected,
                                 size_t index,
                                 int field,
                                 size_t offset,
                                 unsigned char *bytes,
                                 size_t capacity,
                                 DVM_NamespaceObservation *output)
{
    DVM_NamespaceObservation current;
    if (expected && __real_dvm_namespace_observe(group, NULL, 0, 0, 0, NULL, 0, &current) == DVM_NS_OBSERVED)
    {
        return __real_dvm_namespace_observe(group, &current.version, index, field, offset, bytes, capacity, output);
    }
    return __real_dvm_namespace_observe(group, expected, index, field, offset, bytes, capacity, output);
}
