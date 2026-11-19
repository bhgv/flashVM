/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 tokenize support.
 * RU: исходная POSIX-поддержка lib9 tokenize.
 */
#include "lib9.h"

static char qsep[] = " \t\r\n";

/* Compact shell quotes; the fixed separator set contains only ASCII bytes. */


/* [standalone-contract:qtoken]
 * EN: qtoken: original POSIX lib9 tokenize support.
 * RU: qtoken: исходная POSIX-поддержка lib9 tokenize.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sep (char *sep): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *qtoken(char *s, char *sep)
{
    int quoting;
    char *t;

    quoting = 0;
    t = s; /* s is output string, t is input string */
    while (*t != '\0' && (quoting || strchr(sep, (uchar)*t) == nil))
    {
        if (*t != '\'')
        {
            *s++ = *t++;
            continue;
        }
        /* *t is a quote */
        if (!quoting)
        {
            quoting = 1;
            t++;
            continue;
        }
        /* quoting and we're on a quote */
        if (t[1] != '\'')
        {
            /* end of quoted section; absorb closing quote */
            t++;
            quoting = 0;
            continue;
        }
        /* doubled quote; fold two quotes into one */
        t++;
        *s++ = *t++;
    }
    if (*s != '\0')
    {
        *s = '\0';
        if (t == s)
        {
            t++;
        }
    }
    return t;
}

/* Preserve quoted text while finding a complete UTF-8 delimiter rune. */


/* [standalone-contract:etoken]
 * EN: etoken: original POSIX lib9 tokenize support.
 * RU: etoken: исходная POSIX-поддержка lib9 tokenize.
 * t (char *t): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sep (char *sep): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *etoken(char *t, char *sep)
{
    Rune r;
    int quoting, n;

    /* move to end of next token */
    quoting = 0;
    while (*t != '\0')
    {
        n = chartorune(&r, t);
        if (!quoting && utfrune(sep, r) != nil)
        {
            break;
        }
        if (*t != '\'')
        {
            t += n;
            continue;
        }
        /* *t is a quote */
        if (!quoting)
        {
            quoting = 1;
            t++;
            continue;
        }
        /* quoting and we're on a quote */
        if (t[1] != '\'')
        {
            /* end of quoted section; absorb closing quote */
            t++;
            quoting = 0;
            continue;
        }
        /* doubled quote; fold two quotes into one */
        t += 2;
    }
    return t;
}

/* Clear whole delimiter encodings and preserve the historical final remainder. */


/* [standalone-contract:gettokens]
 * EN: gettokens: original POSIX lib9 tokenize support.
 * RU: gettokens: исходная POSIX-поддержка lib9 tokenize.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (char **args): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * maxargs (int maxargs): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * sep (char *sep): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int gettokens(char *s, char **args, int maxargs, char *sep)
{
    Rune r;
    int nargs, n;

    for (nargs = 0; nargs < maxargs; nargs++)
    {
        while (*s != '\0')
        {
            n = chartorune(&r, s);
            if (utfrune(sep, r) == nil)
            {
                break;
            }
            memset(s, 0, n);
            s += n;
        }
        if (*s == '\0')
        {
            break;
        }
        args[nargs] = s;
        s = etoken(s, sep);
    }

    return nargs;
}

/* Split shell words using ASCII whitespace and remove their quoting syntax. */


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
int tokenize(char *s, char **args, int maxargs)
{
    int nargs;

    for (nargs = 0; nargs < maxargs; nargs++)
    {
        while (*s != '\0' && strchr(qsep, (uchar)*s) != nil)
        {
            s++;
        }
        if (*s == '\0')
        {
            break;
        }
        args[nargs] = s;
        s = qtoken(s, qsep);
    }

    return nargs;
}
