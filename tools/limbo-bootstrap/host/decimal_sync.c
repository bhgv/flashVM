/* Copyright (c) 2026 bhgv
 * EN: Serialize shared Gay decimal caches while keeping returned digits caller-owned.
 * RU: Сериализовать общие decimal-кэши Gay, сохраняя владение цифрами у вызывающего.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern char *limbo_decimal_dtoa_unlocked(double, int, int, int *, int *, char **);
extern double limbo_decimal_strtod_unlocked(const char *, char **);
extern void limbo_decimal_free_unlocked(char *);

static pthread_mutex_t decimal_mutex = PTHREAD_MUTEX_INITIALIZER; /* Only decimal native caches, never VM execution. */


/* EN: Require a successful native mutex operation; status is its copied return.
 * Invalid mutex state terminates instead of accessing shared cache unprotected.
 * RU: Требовать успеха native mutex; status — копия результата. При неверном
 * состоянии завершить процесс вместо обращения к кэшу без защиты. */
static void checked(int status)
{
    if (status)
    {
        fprintf(stderr, "native decimal cache mutex failure: %d\n", status);
        abort();
    }
}


/* EN: Parse borrowed text and optionally set borrowed end. Preserve original
 * conversion/errno/rounding semantics; all shared decimal caches use one mutex.
 * RU: Разобрать заимствованный text и при необходимости заполнить end. Сохранить
 * прежнюю семантику преобразования/errno/округления; кэши защищены одним mutex. */
double strtod(const char *text, char **end)
{
    checked(pthread_mutex_lock(&decimal_mutex));
    double result = limbo_decimal_strtod_unlocked(text, end);
    checked(pthread_mutex_unlock(&decimal_mutex));
    return result;
}


/* EN: Convert value under the native cache mutex; mode/digits select the original
 * algorithm. point/sign/end are borrowed outputs. Return private owned digits
 * for the caller to read outside the mutex and release with freedtoa.
 * RU: Преобразовать value под mutex кэша; mode/digits выбирают исходный алгоритм.
 * point/sign/end — заимствованные выходы. Вернуть личные цифры для чтения вне
 * mutex и освобождения вызывающим через freedtoa. */
char *dtoa(double value, int mode, int digits, int *point, int *sign, char **end)
{
    checked(pthread_mutex_lock(&decimal_mutex));
    char *result = limbo_decimal_dtoa_unlocked(value, mode, digits, point, sign, end);
    checked(pthread_mutex_unlock(&decimal_mutex));
    return result;
}


/* EN: Consume the caller's owned digits with the original cache return operation.
 * No VM roots/callbacks are involved; digits follow the original non-NULL contract.
 * RU: Передать личные digits исходной операции возврата в кэш. Здесь нет корней
 * VM/callback; digits соответствует исходному контракту ненулевого указателя. */
void freedtoa(char *digits)
{
    checked(pthread_mutex_lock(&decimal_mutex));
    limbo_decimal_free_unlocked(digits);
    checked(pthread_mutex_unlock(&decimal_mutex));
}
