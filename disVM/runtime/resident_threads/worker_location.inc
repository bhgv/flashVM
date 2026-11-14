/* Copyright (c) 2026 bhgv
 * EN: Owner-only copied location publication; no module pointer survives capture.
 * RU: Публикация location только владельцем; module pointer не переживает capture.
 */


/* EN/RU: Clear old boundary before claiming new guest execution; controller has no slot.
 * Убрать старую границу перед новым guest execution; у controller нет слота. */
void dvm_resident_location_clear(void)
{
    if (current_guest)
    {
        memset(&current_guest->location, 0, sizeof(current_guest->location));
    }
}


/* EN: Publish private native copy on its permanent owner. Caller sampled owned
 * rooted metadata while ACTIVE; no pointers/ownership transfer. Sequence saturates.
 * RU: Опубликовать private native copy на permanent owner. Caller снял собственные
 * rooted metadata в ACTIVE; без pointers/передачи владения. Sequence насыщается. */
void dvm_resident_location_publish(const DVM_DiagnosticLocation *location)
{
    if (current_guest)
    {
        increment_resident_counter(&current_guest->location_sequence);
        current_guest->location = *location;
        current_guest->location.sequence = current_guest->location_sequence;
    }
}
