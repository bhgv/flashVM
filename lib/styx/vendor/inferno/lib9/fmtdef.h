/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fmtdef support.
 * RU: исходная POSIX-поддержка lib9 fmtdef.
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
/*
 * dofmt -- format to a buffer
 * the number of characters formatted is returned,
 * or -1 if there was an error.
 * if the buffer is ever filled, flush is called.
 * it should reset the buffer and return whether formatting should continue.
 */

typedef int (*Fmts)(Fmt *);

typedef struct Quoteinfo Quoteinfo;

struct Quoteinfo
{
    int quoted;    /* if set, string must be quoted */
    int nrunesin;  /* number of input runes that can be accepted */
    int nbytesin;  /* number of input bytes that can be accepted */
    int nrunesout; /* number of runes that will be generated */
    int nbytesout; /* number of bytes that will be generated */
};

/* [standalone-contract:_fmtflush]
 * EN: _fmtflush: original POSIX lib9 dofmt support.
 * RU: _fmtflush: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * t (void *t): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * len (int len): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: void *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *_fmtflush(Fmt *, void *, int);


/* [standalone-contract:_fmtdispatch]
 * EN: _fmtdispatch: original format conversion registry and native callback publication.
 * RU: _fmtdispatch: исходный реестр formatter и публикация нативных callbacks.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (void *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * isrunes (int isrunes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: void *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *_fmtdispatch(Fmt *, void *, int);


/* [standalone-contract:_floatfmt]
 * EN: _floatfmt: original POSIX lib9 fltfmt support.
 * RU: _floatfmt: исходная POSIX-поддержка lib9 fltfmt.
 * fmt (Fmt *fmt): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * f (double f): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _floatfmt(Fmt *, double);


/* [standalone-contract:_fmtpad]
 * EN: _fmtpad: original POSIX lib9 dofmt support.
 * RU: _fmtpad: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtpad(Fmt *, int);


/* [standalone-contract:_rfmtpad]
 * EN: _rfmtpad: original POSIX lib9 dofmt support.
 * RU: _rfmtpad: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _rfmtpad(Fmt *, int);


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
int _fmtFdFlush(Fmt *);


/* [standalone-contract:_efgfmt]
 * EN: _efgfmt: original POSIX lib9 fltfmt support.
 * RU: _efgfmt: исходная POSIX-поддержка lib9 fltfmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _efgfmt(Fmt *);


/* [standalone-contract:_charfmt]
 * EN: _charfmt: original POSIX lib9 dofmt support.
 * RU: _charfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _charfmt(Fmt *);


/* [standalone-contract:_countfmt]
 * EN: _countfmt: original POSIX lib9 dofmt support.
 * RU: _countfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _countfmt(Fmt *);


/* [standalone-contract:_flagfmt]
 * EN: _flagfmt: original POSIX lib9 dofmt support.
 * RU: _flagfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _flagfmt(Fmt *);


/* [standalone-contract:_percentfmt]
 * EN: _percentfmt: original POSIX lib9 dofmt support.
 * RU: _percentfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _percentfmt(Fmt *);


/* [standalone-contract:_ifmt]
 * EN: _ifmt: original POSIX lib9 dofmt support.
 * RU: _ifmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _ifmt(Fmt *);


/* [standalone-contract:_runefmt]
 * EN: _runefmt: original POSIX lib9 dofmt support.
 * RU: _runefmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _runefmt(Fmt *);


/* [standalone-contract:_runesfmt]
 * EN: _runesfmt: original POSIX lib9 dofmt support.
 * RU: _runesfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _runesfmt(Fmt *);


/* [standalone-contract:_strfmt]
 * EN: _strfmt: original POSIX lib9 dofmt support.
 * RU: _strfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _strfmt(Fmt *);


/* [standalone-contract:_badfmt]
 * EN: _badfmt: original POSIX lib9 dofmt support.
 * RU: _badfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _badfmt(Fmt *);


/* [standalone-contract:_fmtdecode]
 * EN: _fmtdecode: original POSIX lib9 dofmt support.
 * RU: _fmtdecode: исходная POSIX-поддержка lib9 dofmt.
 * r (Rune *r): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtdecode(Rune *, char *, int);


/* [standalone-contract:_fmtcpy]
 * EN: _fmtcpy: original POSIX lib9 dofmt support.
 * RU: _fmtcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * vm (void *vm): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * sz (int sz): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtcpy(Fmt *, void *, int, int);


/* [standalone-contract:_fmtrcpy]
 * EN: _fmtrcpy: original POSIX lib9 dofmt support.
 * RU: _fmtrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * vm (void *vm): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtrcpy(Fmt *, void *, int n);


/* [standalone-contract:_fmtlock]
 * EN: _fmtlock: posix/fmtlock implementation and native state.
 * RU: _fmtlock: реализация posix/fmtlock и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _fmtlock(void);


/* [standalone-contract:_fmtunlock]
 * EN: _fmtunlock: posix/fmtlock implementation and native state.
 * RU: _fmtunlock: реализация posix/fmtlock и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _fmtunlock(void);

#define FMTCHAR(f, t, s, c)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (t >= (char *)s)                                                                                            \
        {                                                                                                              \
            t = _fmtflush(f, t, 1);                                                                                    \
            if (t != nil)                                                                                              \
                s = f->stop;                                                                                           \
            else                                                                                                       \
                return -1;                                                                                             \
        }                                                                                                              \
        *t++ = c;                                                                                                      \
    } while (0)

#define FMTRCHAR(f, t, s, c)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (t >= (Rune *)s)                                                                                            \
        {                                                                                                              \
            t = _fmtflush(f, t, sizeof(Rune));                                                                         \
            if (t != nil)                                                                                              \
                s = f->stop;                                                                                           \
            else                                                                                                       \
                return -1;                                                                                             \
        }                                                                                                              \
        *t++ = c;                                                                                                      \
    } while (0)

#define FMTRUNE(f, t, s, r)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        Rune _rune;                                                                                                    \
        int _runelen;                                                                                                  \
        if ((char *)s - t < UTFmax && (_runelen = runelen(r)) > (char *)s - t)                                         \
        {                                                                                                              \
            t = _fmtflush(f, t, _runelen);                                                                             \
            if (t != nil)                                                                                              \
                s = f->stop;                                                                                           \
            else                                                                                                       \
                return -1;                                                                                             \
        }                                                                                                              \
        if (r < Runeself)                                                                                              \
            *t++ = r;                                                                                                  \
        else                                                                                                           \
        {                                                                                                              \
            _rune = r;                                                                                                 \
            t += runetochar(t, &_rune);                                                                                \
        }                                                                                                              \
    } while (0)

#ifndef va_copy
#define va_copy(a, b) ((a) = (b))
#endif
