/* Copyright (c) 2026 bhgv
 * EN: Bounded copied names and exact unsigned32 link signatures, independent of VM layout and input order.
 * RU: Bounded copied names и точные unsigned32 link signatures, независимо от VM layout/input order.
 */
#include "contract.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Private fixed-width name storage; no borrowed pointer survives construction.
 * Private storage имён; borrowed pointer не переживает construction.
 */
typedef struct Symbol
{
    char name[DVM_LAUNCH_SYMBOL_NAME]; /* Owned terminated UTF-8 member name. */
    unsigned long signature; /* Exact validated unsigned32 bits, never a native frame description. */
} Symbol;


/* EN/RU: One native allocation owns immutable sorted entries; shared readers require caller-owned lifetime.
 * Одна native allocation владеет immutable sorted entries; shared readers требуют lifetime от caller.
 */
struct DVM_LaunchContract
{
    size_t count; /* Validated 0..DVM_LAUNCH_SYMBOL_LIMIT. */
    Symbol entries[]; /* Inline copied names/signatures, no managed roots. */
};


/* EN/RU: Compare borrowed native entries for qsort; negative/zero/positive by exact byte name, no state changes.
 * Сравнить borrowed native entries для qsort; negative/zero/positive по byte name, без изменений state.
 */
static int compare(const void *left, const void *right)
{
    const Symbol *a = left;
    const Symbol *b = right;
    return strcmp(a->name, b->name);
}


/* EN/RU: See contract.h; validate all extents before copy and reject ambiguous duplicate exports.
 * См. contract.h; проверить extents до copy, отвергнуть неоднозначные duplicate exports.
 */
DVM_LaunchContract *dvm_launch_contract_create(const DVM_LaunchSymbol *symbols, size_t count)
{
    if (count > DVM_LAUNCH_SYMBOL_LIMIT || (count && !symbols))
    {
        return NULL;
    }
    DVM_LaunchContract *contract = calloc(1, sizeof(*contract) + count * sizeof(Symbol));
    if (!contract)
    {
        return NULL;
    }
    contract->count = count;
    for (size_t i = 0; i < count; i++)
    {
        size_t length = 0;
        if (symbols[i].name)
        {
            while (length < DVM_LAUNCH_SYMBOL_NAME && symbols[i].name[length])
            {
                length++;
            }
        }
        if (!length || length == DVM_LAUNCH_SYMBOL_NAME || symbols[i].signature > 0xffffffffUL)
        {
            free(contract);
            return NULL;
        }
        memcpy(contract->entries[i].name, symbols[i].name, length + 1);
        contract->entries[i].signature = symbols[i].signature;
    }
    qsort(contract->entries, count, sizeof(Symbol), compare);
    for (size_t i = 1; i < count; i++)
    {
        if (!strcmp(contract->entries[i - 1].name, contract->entries[i].name))
        {
            free(contract);
            return NULL;
        }
    }
    return contract;
}


/* EN/RU: See contract.h; validated native count bounds allocation/copy, no mutable state or callback pins.
 * См. contract.h; validated native count ограничивает allocation/copy, без mutable state/callback pins.
 */
DVM_LaunchContract *dvm_launch_contract_clone(const DVM_LaunchContract *contract)
{
    if (!contract)
    {
        return NULL;
    }
    size_t bytes = sizeof(*contract) + contract->count * sizeof(Symbol);
    DVM_LaunchContract *copy = malloc(bytes);
    if (copy)
    {
        memcpy(copy, contract, bytes);
    }
    return copy;
}


/* EN/RU: See contract.h; immutable count read only, NULL is not a valid empty contract for matching.
 * См. contract.h; только immutable count read, NULL не является valid empty contract для matching.
 */
size_t dvm_launch_contract_count(const DVM_LaunchContract *contract)
{
    return contract ? contract->count : 0;
}


/* EN/RU: See contract.h; success borrows the stored name while copying its exact signature.
 * См. contract.h; success заимствует stored name и копирует точную signature.
 */
int dvm_launch_contract_symbol(const DVM_LaunchContract *contract, size_t index, DVM_LaunchSymbol *output)
{
    if (!contract || !output || index >= contract->count)
    {
        return 0;
    }
    output->name = contract->entries[index].name;
    output->signature = contract->entries[index].signature;
    return 1;
}


/* EN/RU: See contract.h; merge sorted lists without allocation, preserving Inferno's required-subset semantics.
 * См. contract.h; merge sorted lists без allocation с required-subset semantics Inferno.
 */
int dvm_launch_contract_matches(const DVM_LaunchContract *required, const DVM_LaunchContract *available)
{
    if (!required || !available)
    {
        return 0;
    }
    size_t cursor = 0;
    for (size_t i = 0; i < required->count; i++)
    {
        while (cursor < available->count && strcmp(available->entries[cursor].name, required->entries[i].name) < 0)
        {
            cursor++;
        }
        if (cursor == available->count || strcmp(required->entries[i].name, available->entries[cursor].name) ||
            required->entries[i].signature != available->entries[cursor].signature)
        {
            return 0;
        }
        cursor++;
    }
    return 1;
}


/* EN/RU: See contract.h; native allocation only, no destructor can invoke a guest.
 * См. contract.h; только native allocation, destructor не вызывает guest.
 */
void dvm_launch_contract_free(DVM_LaunchContract *contract)
{
    free(contract);
}
