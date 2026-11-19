/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 vfprint support.
 * RU: исходная POSIX-поддержка lib9 vfprint.
 */
/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include "lib9.h"
#include "fmtdef.h"


#ifdef ANDROID
#include <android/log.h>

#define LOG_TAG "Inferno OS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif


/* Validate byte-view extents before writes and retain only an unwritten suffix on failure. */


/* [standalone-contract:_fmtFdFlush]
 * EN: _fmtFdFlush: original POSIX lib9 vfprint support.
 * RU: _fmtFdFlush: исходная POSIX-поддержка lib9 vfprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtFdFlush(Fmt *f)
{
    char *p;
    int n, fd;
    long nr;

    if (f == nil || f->runes || f->start == nil || f->stop == nil || (uintptr)f->start == (uintptr)-1 ||
        (uintptr)f->to < (uintptr)f->start || (uintptr)f->stop < (uintptr)f->to ||
        (uintptr)f->stop - (uintptr)f->start > 0x7fffffff)
    {
        return 0;
    }
    p = f->start;
    n = (uintptr)f->to - (uintptr)p;
    fd = (int)(uintptr)f->farg;
#ifdef ANDROID
    /* Log only the occupied span; a full formatting buffer has no spare NUL. */
    if (fd == 1 || fd == 2)
    {
        if (n > 0)
        {
            if (fd == 1)
            {
                LOGI("%.*s", n, p);
            }
            else
            {
                LOGE("%.*s", n, p);
            }
        }
#ifndef DIS_NATIVE64_SCALAR_BOOT
        f->to = f->start;
        return 1;
#endif
        /* The native64 APK probe also records its bounded result on the redirected descriptor. */
    }
#endif
    while (n > 0)
    {
        nr = write(fd, p, n);
        if (nr <= 0 || nr > n)
        {
            memmove(f->start, p, n);
            f->to = (char *)f->start + n;
            return 0;
        }
        p += nr;
        n -= nr;
    }
    f->to = f->start;
    return 1;
}

/* Dispatch only a valid format, then report final native write failures. */


/* [standalone-contract:vfprint]
 * EN: vfprint: original POSIX lib9 vfprint support.
 * RU: vfprint: исходная POSIX-поддержка lib9 vfprint.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int vfprint(int fd, char *fmt, va_list args)
{
    Fmt f;
    char buf[256];
    int n;

    if (fmt == nil || (uintptr)fmt == (uintptr)-1 || fmtfdinit(&f, fd, buf, sizeof(buf)) < 0)
    {
        return -1;
    }
    va_copy(f.args, args);
    n = dofmt(&f, fmt);
    va_end(f.args);
    if (n > 0 && _fmtFdFlush(&f) == 0)
    {
        return -1;
    }
    return n;
}
