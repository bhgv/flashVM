/* Copyright (c) 2026 bhgv
 * EN: Real parallel decimal parsing/formatting with exact IEEE bits and owned digits.
 * RU: Настоящий параллельный decimal parse/format с точными IEEE-битами и личными цифрами.
 */
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *dtoa(double, int, int, int *, int *, char **);
extern void freedtoa(char *);
extern char *g_fmt(char *, double, int);

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Start coordination only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Ready/start predicate notifications. */
static unsigned ready; /* Readers waiting to start, under gate. */
static int started; /* Main released all readers, under gate. */


/* EN: Immutable decimal cases with independently specified binary64 bits.
 * RU: Неизменные decimal-случаи с независимо заданными битами binary64. */
typedef struct Sample
{
    const char *text; /* Borrowed process-lifetime decimal input. */
    uint64_t bits; /* Exact IEEE binary64 result, including signed zero/subnormal. */
} Sample;

static const Sample samples[] = {
    {"0.1", UINT64_C(0x3fb999999999999a)},
    {"3.141592653589793", UINT64_C(0x400921fb54442d18)},
    {"2.2250738585072014e-308", UINT64_C(0x0010000000000000)},
    {"1.7976931348623157e308", UINT64_C(0x7fefffffffffffff)},
    {"4.9406564584124654e-324", UINT64_C(0x0000000000000001)},
    {"9007199254740991", UINT64_C(0x433fffffffffffff)},
    {"-0", UINT64_C(0x8000000000000000)},
    {"1", UINT64_C(0x3ff0000000000000)},
}; /* Shared read-only oracle; no production cache is replaced. */


/* EN: Require condition or terminate with borrowed diagnostic message.
 * RU: Проверить condition либо завершить с заимствованным диагностическим текстом. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL decimal threads: %s\n", message);
        exit(12);
    }
}


/* EN: Copy borrowed binary64 value into integer bits without numeric conversion.
 * RU: Скопировать заимствованный binary64 в целые биты без числового преобразования. */
static uint64_t bits(double value)
{
    uint64_t result;
    require(sizeof(result) == sizeof(value), "binary64 host representation");
    memcpy(&result, &value, sizeof(result));
    return result;
}


/* EN: Parse and round-trip cases concurrently; argument borrows thread index
 * until join. dtoa's returned digits must stay private until this caller frees them.
 * RU: Параллельно разбирать/форматировать случаи; argument заимствует индекс
 * до join. Возвращённые dtoa цифры личные до freedtoa вызывающим. */
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
    unsigned round;
    for (round = 0; round < 4000; round++)
    {
        const Sample *sample = &samples[(round + id) % 8];
        char *end;
        double value = strtod(sample->text, &end);
        require(!*end && bits(value) == sample->bits, "parallel strtod preserves exact input bits");
        char formatted[64];
        g_fmt(formatted, value, 'e');
        double copied = strtod(formatted, &end);
        require(!*end && bits(copied) == sample->bits, "parallel g_fmt/strtod preserves round-trip bits");
        int point;
        int sign;
        char *last;
        char *digits = dtoa(value, 0, 0, &point, &sign, &last);
        require(digits && last >= digits && last - digits < 64 && *last == 0,
            "dtoa returns bounded independently owned digits");
        char snapshot[64];
        memcpy(snapshot, digits, (size_t)(last - digits) + 1);
        sched_yield();
        require(!strcmp(snapshot, digits), "peer conversion cannot overwrite owned dtoa digits");
        freedtoa(digits);
    }
    return NULL;
}


/* EN: Start eight real OS threads together; require complete ownership-safe joins.
 * RU: Вместе запустить восемь потоков ОС и дождаться корректного завершения. */
int main(void)
{
    pthread_t threads[8];
    unsigned ids[8];
    unsigned index;
    for (index = 0; index < 8; index++)
    {
        ids[index] = index;
        require(!pthread_create(&threads[index], NULL, execute, &ids[index]), "reader starts");
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
        require(!pthread_join(threads[index], NULL), "reader joins");
    }
    puts("PASS decimal threads workers=8 cases=32000 exact_bits=1 owned_digits=1");
    return 0;
}
