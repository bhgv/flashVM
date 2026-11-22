/* Copyright (c) 2026 bhgv
 * EN: Exercise exact fid ownership, cancellation and message lifetime in a record pipe.
 * RU: Проверка владения fid, отмены и времени жизни записи в файловом канале.
 */
#include "cancel.h"
#include "record_pipe.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Fail with the precise contract that the tested exchange violated.
 * RU: Завершить проверку с указанием нарушенного контракта обмена. */
static void require(int passed, const char *message)
{
    if (!passed)
    {
        fprintf(stderr, "record pipe: %s\n", message);
        exit(1);
    }
}


/* EN: Read one receipt and verify its value on the producer's exact fid.
 * RU: Прочитать одно подтверждение и проверить байт на точном fid отправителя. */
static void receipt(DVM_VfsHandle *sender, unsigned expected)
{
    unsigned char byte = 255;
    DVM_IoResult got = dvm_vfs_read_chunk(sender, &byte, 1);
    require(got.status == FVM_OK && got.count == 1 && byte == expected, "wrong receipt");
}


/* EN: Cover retirement, cancelled I/O, whole-packet size and explicit acknowledgement.
 * RU: Проверить закрытие, отмену, размер целой записи и явное подтверждение. */
void dvm_test_record_pipe(void)
{
    unsigned char wire[32], bytes[32];
    size_t size;
    require(fvm_srecord_write(1, "limbo", 5, wire, sizeof(wire), &size) == FVM_S_OK, "fixture framing");
    DVM_Provider *pipe = dvm_record_pipe();
    DVM_VfsHandle sender = {0}, competitor = {0}, receiver = {0};
    require(pipe && dvm_provider_open(pipe, "send", "rw", &sender) == FVM_OK &&
        dvm_provider_open(pipe, "send", "rw", &competitor) == FVM_OK &&
        dvm_provider_open(pipe, "recv", "rw", &receiver) == FVM_OK, "missing endpoints");

    require(dvm_vfs_write_chunk(&sender, wire, size).status == FVM_OK, "cannot stage first record");
    require(dvm_vfs_write_chunk(&competitor, wire, size).status != FVM_OK, "parallel sender replaced record");
    dvm_vfs_close(&sender);
    require(dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes)).count == 0, "retired sender left an offer");

    require(dvm_vfs_write_chunk(&competitor, wire, size).status == FVM_OK, "slot not reusable after close");
    require(dvm_vfs_read_chunk(&receiver, bytes, 1).status != FVM_OK, "short read consumed a prefix");
    require(dvm_vfs_read_chunk(&competitor, bytes, 1).count == 0, "premature delivery acknowledgement");

    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    require(cancel && dvm_cancel_enter(&scope, cancel), "no cancel scope");
    dvm_cancel_request(cancel);
    require(dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes)).stop == DVM_IO_CANCELLED, "cancel consumed a packet");
    require(dvm_cancel_leave(&scope), "cannot close cancel scope");
    dvm_cancel_release(cancel);

    DVM_IoResult got = dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes));
    require(got.status == FVM_OK && got.count == size && (got.flags & DVM_IO_RECORD) &&
        !memcmp(wire, bytes, size), "whole record lost");
    dvm_vfs_close(&receiver);
    receipt(&competitor, DVM_RECORD_UNKNOWN);

    require(dvm_provider_open(pipe, "recv", "rw", &receiver) == FVM_OK &&
        dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes)).count == 0, "old offer was redelivered");
    require(dvm_vfs_write_chunk(&competitor, wire, size).status == FVM_OK &&
        dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes)).count == size, "new offer unavailable");
    unsigned char accepted = DVM_RECORD_DELIVERED;
    require(dvm_vfs_write_chunk(&receiver, &accepted, 1).status == FVM_OK, "receiver could not confirm");
    dvm_vfs_close(&receiver);
    receipt(&competitor, DVM_RECORD_DELIVERED);
    require(dvm_provider_open(pipe, "recv", "rw", &receiver) == FVM_OK &&
        dvm_vfs_write_chunk(&competitor, wire, size).status == FVM_OK &&
        dvm_vfs_read_chunk(&receiver, bytes, sizeof(bytes)).count == size, "offer for rejection unavailable");
    unsigned char rejected = DVM_RECORD_REJECTED;
    require(dvm_vfs_write_chunk(&receiver, &rejected, 1).status == FVM_OK, "receiver could not reject");
    receipt(&competitor, DVM_RECORD_REJECTED);
    dvm_vfs_close(&receiver);
    dvm_vfs_close(&competitor);
    dvm_provider_release(pipe);
}
