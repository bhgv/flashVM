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


/* [standalone-contract:checked]
 * EN: checked: host/decimal_sync implementation and native state.
 * RU: checked: реализация host/decimal_sync и нативное состояние.
 * status (int status): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:strtod]
 * EN: strtod: host/decimal_sync implementation and native state.
 * RU: strtod: реализация host/decimal_sync и нативное состояние.
 * text (const char *text): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * end (char **end): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:dtoa]
 * EN: dtoa: host/decimal_sync implementation and native state.
 * RU: dtoa: реализация host/decimal_sync и нативное состояние.
 * value (double value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * digits (int digits): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * point (int *point): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sign (int *sign): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * end (char **end): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:freedtoa]
 * EN: freedtoa: host/decimal_sync implementation and native state.
 * RU: freedtoa: реализация host/decimal_sync и нативное состояние.
 * digits (char *digits): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void freedtoa(char *digits)
{
    checked(pthread_mutex_lock(&decimal_mutex));
    limbo_decimal_free_unlocked(digits);
    checked(pthread_mutex_unlock(&decimal_mutex));
}
