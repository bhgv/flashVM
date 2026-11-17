/* Copyright (c) 2026 bhgv
 * EN: Reuse FlashVM S-record codecs; no private checksum/integer implementation.
 * RU: Использовать FlashVM S-record codecs без собственных checksum/integer codecs.
 */
#include "protocol.h"
#include <string.h>


/* EN/RU: See protocol.h; outputs publish only a complete checked record.
 * См. protocol.h; output публикуется только для полного record. */
int movie_record(unsigned char *out, size_t *size, unsigned kind, const void *bytes, size_t count)
{
    unsigned char type = (unsigned char)kind;
    return fvm_srecord_write_pair(MOVIE_TAG, &type, 1, bytes, count, out,
        MOVIE_FRAME, size) == FVM_S_OK;
}


/* EN/RU: See protocol.h; registry metadata is copied, never a native pointer.
 * См. protocol.h; metadata копируются, native pointer не передаётся. */
int movie_binding(unsigned char *out, size_t *size)
{
    FVM_SBinding binding;
    unsigned char payload[128];
    size_t length;
    memset(&binding, 0, sizeof(binding));
    binding.tag = MOVIE_TAG;
    binding.version = MOVIE_VERSION;
    binding.type_size = sizeof(MOVIE_TYPE)-1;
    memcpy(binding.type, MOVIE_TYPE, sizeof(MOVIE_TYPE));
    return fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &length) == FVM_S_OK &&
        fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, length, out, MOVIE_FRAME, size) == FVM_S_OK;
}


/* EN/RU: See protocol.h; unknown versions/types never reach service dispatch.
 * См. protocol.h; неизвестные versions/types не попадают в service dispatch. */
int movie_bound(const FVM_SBindings *bindings)
{
    FVM_SBinding binding;
    return fvm_sbindings_lookup(bindings, MOVIE_TAG, &binding) == FVM_S_OK &&
        binding.version == MOVIE_VERSION && binding.type_size == sizeof(MOVIE_TYPE)-1 &&
        !memcmp(binding.type, MOVIE_TYPE, sizeof(MOVIE_TYPE)-1);
}
