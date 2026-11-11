/* Copyright (c) 2026 bhgv
 * EN: Concurrent formatter publication/lookups with reentrant callback formatting.
 * RU: Параллельная публикация/поиск форматтеров с рекурсивным форматированием callback.
 */
#include "lib9.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test start/publication barriers only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Predicate notifications. */
static unsigned ready; /* Workers ready to register, under gate. */
static unsigned published; /* Workers finished their four registrations, under gate. */
static int started; /* Main releases simultaneous cold registration, under gate. */


/* EN: Require condition or fail with borrowed message.
 * RU: Проверить condition либо завершить с заимствованным message. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL format threads: %s\n", message);
        exit(12);
    }
}


/* EN: Format nested text through borrowed f; return the genuine formatter status.
 * RU: Форматировать вложенный текст через заимствованный f; вернуть настоящий статус. */
static int first(Fmt *f)
{
    return fmtprint(f, "A[%s]", "ok");
}


/* EN: Alternate nested callback, with the same borrowed-f/status contract.
 * RU: Второй вложенный callback с тем же контрактом заимствованного f и статуса. */
static int second(Fmt *f)
{
    return fmtprint(f, "B[%s]", "ok");
}


/* EN: Encode one of the 32 private format runes into caller-owned format[4].
 * RU: Записать одну из 32 личных рун формата в принадлежащий вызывающему format[4]. */
static void encode(char *format, unsigned index)
{
    format[0] = '%';
    format[1] = (char)0xc4;
    format[2] = (char)(0x80 + index);
    format[3] = 0;
}


/* EN: Publish four unique formats, then update/lookup while callbacks reenter.
 * argument borrows the index until join; all output buffers are owner-local.
 * RU: Опубликовать четыре формата, затем менять/читать их при вложенных callback.
 * argument заимствует индекс до join; выходные буферы принадлежат своему потоку. */
static void *execute(void *argument)
{
    unsigned id = *(unsigned *)argument;
    pthread_mutex_lock(&gate);
    ready++;
    pthread_cond_broadcast(&changed);
    while (!started)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    unsigned index;
    for (index = 0; index < 4; index++)
    {
        require(!fmtinstall(0x100 + id * 4 + index, first), "cold formatter publication succeeds");
    }
    pthread_mutex_lock(&gate);
    published++;
    pthread_cond_broadcast(&changed);
    while (published != 8)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    unsigned round;
    for (round = 0; round < 1000; round++)
    {
        require(!fmtinstall(0x100 + id * 4 + round % 4, round % 2 ? first : second),
            "formatter replacement succeeds");
        char format[4];
        char output[64];
        char expected[64];
        encode(format, (round + id) % 32);
        snprint(output, sizeof(output), format);
        require(!strcmp(output, "A[ok]") || !strcmp(output, "B[ok]"),
            "every fully published callback remains callable and reentrant");
        snprint(output, sizeof(output), "%d:%x:%s", (int)id, (int)(0xa0 + id), "fixed");
        snprintf(expected, sizeof(expected), "%u:%x:fixed", id, 0xa0 + id);
        require(!strcmp(output, expected), "lazy builtin lookup preserves owner-local output");
    }
    return NULL;
}


/* EN: Start eight real publishers together; join after all native format checks.
 * RU: Вместе запустить восемь публикаторов и дождаться всех native-проверок. */
int main(void)
{
    pthread_t threads[8];
    unsigned ids[8];
    unsigned index;
    for (index = 0; index < 8; index++)
    {
        ids[index] = index;
        require(!pthread_create(&threads[index], NULL, execute, &ids[index]), "worker starts");
    }
    pthread_mutex_lock(&gate);
    while (ready != 8)
    {
        pthread_cond_wait(&changed, &gate);
    }
    started = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    for (index = 0; index < 8; index++)
    {
        require(!pthread_join(threads[index], NULL), "worker joins");
    }
    puts("PASS format threads workers=8 registrations=8032 reentrant=1 private_output=1");
    return 0;
}
