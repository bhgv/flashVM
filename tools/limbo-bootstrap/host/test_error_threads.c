/* Copyright (c) 2026 bhgv
 * EN: Verify native custom-error text follows the OS thread just like errno.
 * RU: Проверить принадлежность native-текста ошибки потоку ОС, как у errno.
 */
#include "lib9.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test barrier only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* All writers completed first publication. */
static unsigned written; /* Initial native errors published, under gate. */


/* EN: Require condition or terminate with borrowed message.
 * RU: Проверить condition либо завершить с заимствованным message. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL error threads: %s\n", message);
        exit(12);
    }
}


/* EN: Publish/read private native errors around peer writes and OS errno changes.
 * argument borrows the worker index until join; buffers stay on this thread's stack.
 * RU: Публиковать/читать личные native-ошибки при чужих записях и смене OS errno.
 * argument заимствует индекс до join; буферы лежат на стеке этого потока. */
static void *execute(void *argument)
{
    unsigned id = *(unsigned *)argument;
    char expected[ERRMAX];
    char actual[ERRMAX];
    snprintf(expected, sizeof(expected), "native-owner-%u", id);
    werrstr("%s", expected);
    pthread_mutex_lock(&gate);
    written++;
    pthread_cond_broadcast(&changed);
    while (written != 8)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    rerrstr(actual, sizeof(actual));
    require(!strcmp(actual, expected), "custom error remains private after all peer writes");
    unsigned round;
    for (round = 0; round < 2000; round++)
    {
        snprintf(expected, sizeof(expected), "native-owner-%u-round-%u", id, round);
        werrstr("%s", expected);
        sched_yield();
        rerrstr(actual, sizeof(actual));
        require(!strcmp(actual, expected), "custom error follows this thread across scheduling");
        snprint(actual, sizeof(actual), "%r");
        require(!strcmp(actual, expected), "native error formatter reads this thread's custom text");
        errno = EINTR;
        rerrstr(actual, sizeof(actual));
        require(!strcmp(actual, "interrupted"), "OS errno still selects its normal error text");
    }
    return NULL;
}


/* EN: Warm immutable builtin format bindings, then test eight genuine OS owners.
 * RU: Прогреть стандартные форматы и проверить восемь настоящих владельцев ОС. */
int main(void)
{
    werrstr("warm-%s-%d", "error", 0);
    pthread_t threads[8];
    unsigned ids[8];
    unsigned index;
    for (index = 0; index < 8; index++)
    {
        ids[index] = index;
        require(!pthread_create(&threads[index], NULL, execute, &ids[index]), "worker starts");
    }
    for (index = 0; index < 8; index++)
    {
        require(!pthread_join(threads[index], NULL), "worker joins");
    }
    char actual[ERRMAX];
    rerrstr(actual, sizeof(actual));
    require(!strcmp(actual, "warm-error-0"), "child native errors do not change the original thread");
    puts("PASS native error threads workers=8 updates=16000 private_text=1 os_errno=1");
    return 0;
}
