/* Copyright (c) 2026 bhgv
 * EN: Verify exact server-side cancellation and retained effects using the standalone C SDK.
 * RU: Проверить точную отмену на сервере и сохранение эффектов через автономный SDK C.
 */
#include "styxrpc/vfs_client.h"
#include "client.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* EN: Decode the provider fixture's separate status file by byte position.
 * RU: Разобрать позиции байтов отдельного файла состояния провайдера. */
enum
{
    ReadEntered = 0, /* Read callback admission. */
    ReadStopped,     /* Read callback cancellation. */
    WriteEntered,    /* Suffix write admission. */
    WriteStopped,    /* Suffix write cancellation. */
    DataClosed,      /* Exact Data cookie retirements. */
    Prefix,          /* Confirmed first byte. */
    Suffix,          /* Byte at the unconfirmed second position. */
    NewByte,         /* Byte written through Rcreate. */
    Fields           /* Complete gate read size. */
};

/* EN: One SDK caller, with results observed by the supervisor after join.
 * RU: Один вызов SDK, результат которого управляющий читает после join. */
typedef struct Transfer
{
    StyxRpcVfsFile *file;    /* Owned by main, pinned through join. */
    pthread_t thread;       /* Caller runs one SDK transfer and owns result until join. */
    StyxRpcIoOutcome status; /* Final SDK outcome. */
    size_t used;            /* Confirmed bytes, preserved on stop. */
    char bytes[2];          /* Read output or write input for this call. */
    int writing;            /* Select write instead of read. */
    int stop;               /* Atomic flag from main, polled on this caller. */
    int started;            /* Main must join this worker once. */
} Transfer;


/* EN: Sample the specific caller's cooperative stop flag.
 * RU: Проверить флаг отмены точного вызывающего. */
static int stopped(void *state)
{
    Transfer *transfer = state;
    return __atomic_load_n(&transfer->stop, __ATOMIC_ACQUIRE);
}


/* EN: Borrow the file during one positional operation and retain the outcome.
 * RU: Заимствовать файл на одну позиционную операцию и сохранить её результат. */
static void *run(void *state)
{
    Transfer *transfer = state;
    transfer->status = transfer->writing ?
        styxrpc_vfs_file_write_until(transfer->file, 0, transfer->bytes, sizeof(transfer->bytes),
            &transfer->used, stopped, transfer) :
        styxrpc_vfs_file_read_until(transfer->file, 0, transfer->bytes, 1,
            &transfer->used, stopped, transfer);
    return NULL;
}


/* EN: Open one fresh owned fid and launch its caller on a native thread.
 * RU: Открыть свежий собственный fid и запустить его вызов в нативном потоке. */
static int begin(StyxClient *session, Transfer *transfer, int writing)
{
    *transfer = (Transfer){.writing = writing, .bytes = {'A', 'B'}};
    transfer->started = styxrpc_vfs_open(session, "data", writing ? STYXRPC_FILE_WRITE : STYXRPC_FILE_READ,
        &transfer->file) == STYXRPC_IO_OK && !pthread_create(&transfer->thread, NULL, run, transfer);
    return transfer->started;
}


/* EN: Optionally stop, join the caller and retire the exact owned fid.
 * RU: При необходимости отменить вызов, дождаться его и закрыть точный fid. */
static void finish(Transfer *transfer, int cancel)
{
    if (transfer->started)
    {
        if (cancel)
        {
            __atomic_store_n(&transfer->stop, 1, __ATOMIC_RELEASE);
        }
        pthread_join(transfer->thread, NULL);
        transfer->started = 0;
    }
    styxrpc_vfs_close(transfer->file, 0);
    transfer->file = NULL;
}


/* EN: Submit one owner command on an independent fid.
 * RU: Отправить одну команду владельцу через независимый fid. */
static int command(StyxClient *session, char code)
{
    size_t used;
    return styxrpc_vfs_write_at(session, "gate", 0, &code, 1, &used) == STYXRPC_IO_OK && used == 1;
}


/* EN: Read the current status snapshot and wait for one exact phase count.
 * RU: Читать текущий статус и ждать точного счётчика этапа. */
static int await(StyxClient *session, unsigned field, unsigned target, unsigned char *state)
{
    const struct timespec interval = {0, 1000000};
    for (unsigned i = 0; i < 1400; ++i)
    {
        size_t used;
        if (styxrpc_vfs_read_at(session, "gate", 0, state, Fields, &used) != STYXRPC_IO_OK || used != Fields)
        {
            return 0;
        }
        if (state[field] >= target)
        {
            return state[field] == target;
        }
        nanosleep(&interval, NULL);
    }
    return 0;
}


/* EN: Test that a file just created through Tcreate also gets a deferred writer.
 * RU: Проверить отложенную запись через fid только что созданного файла. */
static int new_writer(StyxClient *session)
{
    StyxRpcVfsFile *file = NULL;
    size_t used;
    int result = styxrpc_vfs_create(session, "new", &file) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_write(file, 0, "N", 1, &used) == STYXRPC_IO_OK && used == 1;
    styxrpc_vfs_close(file, 0);
    return result;
}


/* EN: Exercise simultaneous waits, a short write, a stopped cursor, and disconnect.
 * RU: Проверить параллельные ожидания, короткую запись, отмену курсора и разрыв. */
static int exercise(StyxClient *session)
{
    Transfer reader = {0}, writer = {0};
    unsigned char gate[Fields];
    int passed = begin(session, &reader, 0) && begin(session, &writer, 1) &&
        await(session, ReadEntered, 1, gate) && await(session, WriteEntered, 1, gate);
    if (passed)
    {
        finish(&writer, 1);
        passed = writer.status == STYXRPC_IO_CANCELLED && writer.used == 1 &&
            await(session, WriteStopped, 1, gate) &&
            gate[Prefix] == 'A' && gate[Suffix] == '_' && !gate[ReadStopped] && !gate[DataClosed];
    }
    if (passed && command(session, 'R'))
    {
        finish(&reader, 0);
        passed = reader.status == STYXRPC_IO_OK && reader.used == 1 && reader.bytes[0] == 'R' &&
            command(session, 'P') && begin(session, &reader, 0) && await(session, ReadEntered, 2, gate);
    }
    else
    {
        passed = 0;
    }
    if (passed)
    {
        finish(&reader, 1);
        passed = reader.status == STYXRPC_IO_CANCELLED && !reader.used &&
            await(session, ReadStopped, 1, gate) && gate[DataClosed] == 1 &&
            command(session, 'D') && await(session, DataClosed, 3, gate);
    }
    if (passed && command(session, 'R'))
    {
        size_t used;
        char byte;
        passed = styxrpc_vfs_read_at(session, "data", 0, &byte, 1, &used) == STYXRPC_IO_OK &&
            used == 1 && byte == 'R' && new_writer(session) && await(session, NewByte, 'N', gate) &&
            command(session, 'P') && begin(session, &reader, 0) && await(session, ReadEntered, 4, gate);
    }
    else
    {
        passed = 0;
    }
    if (passed)
    {
        styxclientshutdown(session);
        finish(&reader, 0);
        passed = reader.status != STYXRPC_IO_OK && !reader.used;
    }
    else
    {
        command(session, 'D');
    }
    finish(&reader, 1);
    finish(&writer, 1);
    return passed;
}


/* EN: Probe capacity on a separate fid. The SDK reports a sent data request
 * with Rerror conservatively as uncertain and confirms no bytes.
 * RU: Проверить ёмкость отдельным fid. SDK отмечает отправленный запрос с Rerror
 * как неопределённый и не подтверждает ни одного байта. */
static int excess_read(StyxClient *session)
{
    StyxRpcVfsFile *file = NULL;
    char byte;
    size_t used;
    int refused = styxrpc_vfs_open(session, "data", STYXRPC_FILE_READ, &file) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_read(file, 0, &byte, 1, &used) == STYXRPC_IO_UNCERTAIN && !used;
    styxrpc_vfs_close(file, 0);
    return refused;
}


/* EN: Fill the server's active limit, then prove a flushed worker still
 * counts until join. A newly admitted file succeeds after a slot is returned.
 * RU: Заполнить доступные места. Отменённый поток занимает место до join.
 * После освобождения места новый файловый запрос выполняется. */
static int pressure(StyxClient *session)
{
    enum { Limit = 16 }; /* Expected server job budget. */
    Transfer active[Limit] = {{0}};
    Transfer replacement = {0};
    unsigned char gate[Fields];
    int passed = 1;
    for (unsigned i = 0; i < Limit && passed; ++i)
    {
        passed = begin(session, &active[i], 0);
    }
    passed = passed && await(session, ReadEntered, Limit, gate) && excess_read(session);
    if (passed)
    {
        finish(&active[0], 1);
        passed = active[0].status == STYXRPC_IO_CANCELLED &&
            await(session, ReadStopped, 1, gate) && gate[DataClosed] == 1 &&
            excess_read(session) && await(session, ReadEntered, Limit, gate);
    }
    passed = passed && command(session, 'D') && await(session, DataClosed, 3, gate) &&
        begin(session, &replacement, 0) && await(session, ReadEntered, Limit + 1, gate);
    if (command(session, 'R'))
    {
        finish(&replacement, 0);
        passed = passed && replacement.status == STYXRPC_IO_OK && replacement.bytes[0] == 'R';
        for (size_t i = 1; i < Limit; ++i)
        {
            finish(&active[i], 0);
            passed = passed && active[i].status == STYXRPC_IO_OK && active[i].used == 1;
        }
    }
    else
    {
        passed = 0;
    }
    command(session, 'D');
    finish(&replacement, 1);
    for (size_t i = 0; i < Limit; ++i)
    {
        finish(&active[i], 1);
    }
    return passed;
}


/* EN: Mount the inherited connection, run the SDK check and release the session.
 * RU: Смонтировать унаследованное соединение, выполнить проверку SDK и закрыть сессию. */
int main(int argc, char **argv)
{
    if (argc != 2 && (argc != 3 || strcmp(argv[2], "pressure")))
    {
        return 1;
    }
    StyxClient *session = styxclientmountfd(atoi(argv[1]), 4096, "fixture", "", 3000, 32);
    int result = session && (argc == 3 ? pressure(session) : exercise(session));
    styxclientfree(session);
    puts(result ? "PASS standalone C SDK deferred file transfers" : "FAIL standalone C SDK deferred transfers");
    return !result;
}
