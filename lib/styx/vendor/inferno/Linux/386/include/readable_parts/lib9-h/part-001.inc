/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of lib9.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* define _BSD_SOURCE to use ISO C, POSIX, and 4.3BSD things. */
#define USE_PTHREADS
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#define _XOPEN_SOURCE 500
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64
#ifdef USE_PTHREADS
#define _REENTRANT 1
#endif
#include <features.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#define sync __os_sync
#include <unistd.h>
#undef sync
#include <errno.h>
#define __NO_STRING_INLINES
#include <string.h>
#include "math.h"
#include <fcntl.h>
#include <setjmp.h>
#include <float.h>
#include <time.h>
#include <endian.h>

#define getwd infgetwd

#ifndef EMU
typedef struct Proc Proc;
#endif

/*
 * math module dtoa
 * #define __LITTLE_ENDIAN /usr/include/endian.h under linux
 */

#define nil ((void *)0)

typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned int Rune;
typedef long long int vlong;
typedef unsigned long long int uvlong;
typedef unsigned int u32int;
typedef uvlong u64int;

typedef unsigned int mpdigit; /* for /sys/include/mp.h */
typedef unsigned short u16int;
typedef unsigned char u8int;
typedef unsigned long uintptr;

#define USED(x)                                                                                                        \
    if (x)                                                                                                             \
    {                                                                                                                  \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
    }
#define SET(x)

#undef nelem
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#undef offsetof
#define offsetof(s, m) (ulong)(&(((s *)0)->m))
#undef assert
#define assert(x)                                                                                                      \
    if (x)                                                                                                             \
    {                                                                                                                  \
    }                                                                                                                  \
    else                                                                                                               \
        _assert("x")

/*
 * most mem and string routines are declared by ANSI/POSIX files above
 */


/* [standalone-contract:strecpy]
 * EN: strecpy: original POSIX lib9 strecpy support.
 * RU: strecpy: исходная POSIX-поддержка lib9 strecpy.
 * to (char *to): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * e (char *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * from (char *from): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *strecpy(char *, char *, char *);


/* [standalone-contract:strdup]
 * EN: strdup: original POSIX lib9 strdup support.
 * RU: strdup: исходная POSIX-поддержка lib9 strdup.
 * s (const char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *strdup(const char *);


/* [standalone-contract:cistrncmp]
 * EN: cistrncmp: original POSIX lib9 cistrncmp support.
 * RU: cistrncmp: исходная POSIX-поддержка lib9 cistrncmp.
 * s1 (char *s1): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * s2 (char *s2): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int cistrncmp(char *, char *, int);


/* [standalone-contract:cistrcmp]
 * EN: cistrcmp: original POSIX lib9 cistrcmp support.
 * RU: cistrcmp: исходная POSIX-поддержка lib9 cistrcmp.
 * s1 (char *s1): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * s2 (char *s2): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int cistrcmp(char *, char *);


/* [standalone-contract:cistrstr]
 * EN: cistrstr: original POSIX lib9 cistrstr support.
 * RU: cistrstr: исходная POSIX-поддержка lib9 cistrstr.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sub (char *sub): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *cistrstr(char *, char *);


/* [standalone-contract:tokenize]
 * EN: tokenize: original POSIX lib9 tokenize support.
 * RU: tokenize: исходная POSIX-поддержка lib9 tokenize.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (char **args): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * maxargs (int maxargs): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int tokenize(char *, char **, int);

enum
{
    UTFmax = 4,          /* maximum bytes per rune */
    Runesync = 0x80,     /* cannot represent part of a UTF sequence (<) */
    Runeself = 0x80,     /* rune and UTF sequences are the same (<) */
    Runeerror = 0xFFFD,  /* decoding error in UTF */
    Runemax = 0x10FFFF,  /* 21-bit rune */
    Runemask = 0x1FFFFF, /* bits used by runes (see grep) */
};

/*
 * rune routines
 */


/* [standalone-contract:runetochar]
 * EN: runetochar: UTF-8 byte/rune conversion and validity.
 * RU: runetochar: преобразование и проверка UTF-8 byte/rune.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * rune (Rune *rune): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int runetochar(char *, Rune *);


/* [standalone-contract:chartorune]
 * EN: chartorune: UTF-8 byte/rune conversion and validity.
 * RU: chartorune: преобразование и проверка UTF-8 byte/rune.
 * rune (Rune *rune): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int chartorune(Rune *, char *);


/* [standalone-contract:runelen]
 * EN: runelen: UTF-8 byte/rune conversion and validity.
 * RU: runelen: преобразование и проверка UTF-8 byte/rune.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int runelen(long);


/* [standalone-contract:runenlen]
 * EN: runenlen: UTF-8 byte/rune conversion and validity.
 * RU: runenlen: преобразование и проверка UTF-8 byte/rune.
 * r (Rune *r): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * nrune (int nrune): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int runenlen(Rune *, int);


/* [standalone-contract:fullrune]
 * EN: fullrune: UTF-8 byte/rune conversion and validity.
 * RU: fullrune: преобразование и проверка UTF-8 byte/rune.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fullrune(char *, int);


/* [standalone-contract:utflen]
 * EN: utflen: original POSIX lib9 utflen support.
 * RU: utflen: исходная POSIX-поддержка lib9 utflen.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int utflen(char *);


/* [standalone-contract:utfnlen]
 * EN: utfnlen: original POSIX lib9 utfnlen support.
 * RU: utfnlen: исходная POSIX-поддержка lib9 utfnlen.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * m (long m): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int utfnlen(char *, long);


/* [standalone-contract:utfrune]
 * EN: utfrune: original POSIX lib9 utfrune support.
 * RU: utfrune: исходная POSIX-поддержка lib9 utfrune.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: 0, nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0, nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *utfrune(char *, long);


/* [standalone-contract:utfrrune]
 * EN: utfrrune: original POSIX lib9 utfrrune support.
 * RU: utfrrune: исходная POSIX-поддержка lib9 utfrrune.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *utfrrune(char *, long);
extern char *utfutf(char *, char *);


/* [standalone-contract:utfecpy]
 * EN: utfecpy: original POSIX lib9 utfecpy support.
 * RU: utfecpy: исходная POSIX-поддержка lib9 utfecpy.
 * to (char *to): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * e (char *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * from (char *from): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *utfecpy(char *, char *, char *);

extern Rune *runestrcat(Rune *, Rune *);


/* [standalone-contract:runestrchr]
 * EN: runestrchr: original POSIX lib9 runestrchr support.
 * RU: runestrchr: исходная POSIX-поддержка lib9 runestrchr.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Rune *runestrchr(Rune *, Rune);
extern int runestrcmp(Rune *, Rune *);
extern Rune *runestrcpy(Rune *, Rune *);
extern Rune *runestrncpy(Rune *, Rune *, long);
extern Rune *runestrecpy(Rune *, Rune *, Rune *);
extern Rune *runestrdup(Rune *);
extern Rune *runestrncat(Rune *, Rune *, long);
extern int runestrncmp(Rune *, Rune *, long);
extern Rune *runestrrchr(Rune *, Rune);


/* [standalone-contract:runestrlen]
 * EN: runestrlen: original POSIX lib9 runestrlen support.
 * RU: runestrlen: исходная POSIX-поддержка lib9 runestrlen.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern long runestrlen(Rune *);
extern Rune *runestrstr(Rune *, Rune *);


/* [standalone-contract:tolowerrune]
 * EN: tolowerrune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: tolowerrune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Rune tolowerrune(Rune);


/* [standalone-contract:totitlerune]
 * EN: totitlerune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: totitlerune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Rune totitlerune(Rune);


/* [standalone-contract:toupperrune]
 * EN: toupperrune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: toupperrune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Rune toupperrune(Rune);


/* [standalone-contract:isalpharune]
 * EN: isalpharune: original POSIX lib9 runetypebody-6.2.0 support.
 * RU: isalpharune: исходная POSIX-поддержка lib9 runetypebody-6.2.0.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int isalpharune(Rune);
