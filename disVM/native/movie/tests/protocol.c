/* Copyright (c) 2026 bhgv
 * EN: Service framing refusal and independent open ownership without a guest or network.
 * RU: Отказы service framing и независимое владение opens без guest/network.
 */
#include "service.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Keep release-build fixture checks explicit; fail before continuing a broken contract.
 * Сохранять явные release-build проверки; завершиться при нарушенном contract. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "movie protocol: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Open a new independently owned stream into empty output from borrowed service.
 * Открыть независимо владеющий stream borrowed service в empty output. */
static void opened(DVM_Provider *provider, DVM_VfsHandle *out)
{
    check(dvm_provider_open(provider, ".", "rw", out) == FVM_OK, "duplex open");
}


/* EN/RU: Exercise bad CRC, missing binding, invalid purpose and bounds before resource resolution.
 * Fake native root is never pinned/opened: every request must refuse before VFS lookup.
 * Проверить bad CRC/binding/purpose/bounds до VFS lookup; fake root не открывается. */
int main(void)
{
    DVM_Namespace *group = dvm_namespace_create("/movie-protocol-no-native-root", "/", "/");
    DVM_Provider *provider = group ? styxmovieservice(group) : NULL;
    unsigned char binding[MOVIE_FRAME], buffer[MOVIE_FRAME], received[MOVIE_FRAME];
    size_t size, count;
    check(provider && movie_binding(binding, &size), "service/declaration");
    check(dvm_provider_bindable_at(provider, "."), "service file cannot be bound");
    DVM_Provider *captured = dvm_provider_capture(provider, ".");
    check(captured == provider, "service root capture changed endpoint");
    dvm_provider_release(captured);
    DVM_VfsHandle first = {0}, second = {0};
    opened(provider, &first);
    opened(provider, &second);
    dvm_namespace_release(group);
    for (size_t i = 0; i < size; i++)
    {
        check(dvm_vfs_write(&first, binding+i, 1) == FVM_OK, "fragmented declaration");
    }
    DVM_VfsHandle combined = {0};
    opened(provider, &combined);
    memcpy(buffer, binding, size);
    memcpy(buffer+size, binding, size);
    check(dvm_vfs_write(&combined, buffer, size*2) == FVM_OK, "coalesced declarations");
    dvm_vfs_close(&combined);
    memcpy(buffer, binding, size);
    buffer[0] ^= 1;
    check(dvm_vfs_write(&second, buffer, size) != FVM_OK, "bad CRC accepted");
    check(dvm_vfs_read(&first, received, 0, &count) == FVM_OK && !count, "zero read");
    for (size_t i = 0; i < size; i++)
    {
        check(dvm_vfs_read(&first, received+i, 1, &count) == FVM_OK && count == 1,
            "independent fragmented response");
    }
    check(!memcmp(received, binding, size), "response mutated or zero read consumed data");
    unsigned char invalid[] = {99, 1, 0, 0, 0, 'x'};
    check(movie_record(buffer, &size, MOVIE_REQUEST, invalid, sizeof(invalid)), "invalid purpose frame");
    check(dvm_vfs_write(&first, buffer, size) != FVM_OK, "unknown purpose accepted");
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    opened(provider, &first);
    check(dvm_vfs_write(&first, buffer, size) != FVM_OK, "unbound request accepted");
    dvm_vfs_close(&first);
    opened(provider, &first);
    check(dvm_vfs_write(&first, buffer, MOVIE_FRAME+1) != FVM_OK, "oversized frame copied");
    dvm_vfs_close(&first);
    dvm_provider_release(provider);
    puts("PASS movie service framing ownership and refusal");
    return 0;
}
