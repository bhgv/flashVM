/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of lib9.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
extern int fmtstrinit(Fmt *);


/* [standalone-contract:fmtstrflush]
 * EN: fmtstrflush: original POSIX lib9 fmtstr support.
 * RU: fmtstrflush: исходная POSIX-поддержка lib9 fmtstr.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *fmtstrflush(Fmt *);


/* [standalone-contract:runefmtstrinit]
 * EN: runefmtstrinit: original POSIX lib9 vsmprint support.
 * RU: runefmtstrinit: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int runefmtstrinit(Fmt *);


/* [standalone-contract:runefmtstrflush]
 * EN: runefmtstrflush: original POSIX lib9 fmtstr support.
 * RU: runefmtstrflush: исходная POSIX-поддержка lib9 fmtstr.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: Rune *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Rune *runefmtstrflush(Fmt *);


/* [standalone-contract:fmtinstall]
 * EN: fmtinstall: original format conversion registry and native callback publication.
 * RU: fmtinstall: исходный реестр formatter и публикация нативных callbacks.
 * c (int c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * f (Fmts f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtinstall(int, int (*)(Fmt *));


/* [standalone-contract:dofmt]
 * EN: dofmt: original POSIX lib9 dofmt support.
 * RU: dofmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dofmt(Fmt *, char *);


/* [standalone-contract:dorfmt]
 * EN: dorfmt: original POSIX lib9 dorfmt support.
 * RU: dorfmt: исходная POSIX-поддержка lib9 dorfmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (Rune *fmt): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dorfmt(Fmt *, Rune *);


/* [standalone-contract:fmtprint]
 * EN: fmtprint: original POSIX lib9 fmtprint support.
 * RU: fmtprint: исходная POSIX-поддержка lib9 fmtprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtprint(Fmt *, char *, ...);


/* [standalone-contract:fmtvprint]
 * EN: fmtvprint: original POSIX lib9 fmtvprint support.
 * RU: fmtvprint: исходная POSIX-поддержка lib9 fmtvprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtvprint(Fmt *, char *, va_list);


/* [standalone-contract:fmtrune]
 * EN: fmtrune: original POSIX lib9 fmtrune support.
 * RU: fmtrune: исходная POSIX-поддержка lib9 fmtrune.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * r (int r): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtrune(Fmt *, int);


/* [standalone-contract:fmtstrcpy]
 * EN: fmtstrcpy: original POSIX lib9 dofmt support.
 * RU: fmtstrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtstrcpy(Fmt *, char *);


/* [standalone-contract:fmtrunestrcpy]
 * EN: fmtrunestrcpy: original POSIX lib9 dofmt support.
 * RU: fmtrunestrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int fmtrunestrcpy(Fmt *, Rune *);
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */


/* [standalone-contract:errfmt]
 * EN: errfmt: original POSIX lib9 errfmt support.
 * RU: errfmt: исходная POSIX-поддержка lib9 errfmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int errfmt(Fmt *f);

/*
 * quoted strings
 */
extern char *unquotestrdup(char *);
extern Rune *unquoterunestrdup(Rune *);
extern char *quotestrdup(char *);
extern Rune *quoterunestrdup(Rune *);


/* [standalone-contract:quotestrfmt]
 * EN: quotestrfmt: original POSIX lib9 fmtquote support.
 * RU: quotestrfmt: исходная POSIX-поддержка lib9 fmtquote.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int quotestrfmt(Fmt *);


/* [standalone-contract:quoterunestrfmt]
 * EN: quoterunestrfmt: original POSIX lib9 fmtquote support.
 * RU: quoterunestrfmt: исходная POSIX-поддержка lib9 fmtquote.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int quoterunestrfmt(Fmt *);


/* [standalone-contract:quotefmtinstall]
 * EN: quotefmtinstall: original POSIX lib9 fmtquote support.
 * RU: quotefmtinstall: исходная POSIX-поддержка lib9 fmtquote.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void quotefmtinstall(void);
extern int (*doquote)(int);

/*
 * random number
 */
extern ulong truerand(void);
extern ulong ntruerand(ulong);

/*
 * math
 */


/* [standalone-contract:isNaN]
 * EN: isNaN: original POSIX lib9 isnan-posix support.
 * RU: isNaN: исходная POSIX-поддержка lib9 isnan-posix.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int isNaN(double);
extern double NaN(void);


/* [standalone-contract:isInf]
 * EN: isInf: original POSIX lib9 isnan-posix support.
 * RU: isInf: исходная POSIX-поддержка lib9 isnan-posix.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * sign (int sign): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int isInf(double, int);

/*
 * Time-of-day
 */

typedef struct Tm Tm;

struct Tm
{
    int sec;
    int min;
    int hour;
    int mday;
    int mon;
    int year;
    int wday;
    int yday;
    char zone[4];
    int tzoff;
};

extern vlong osnsec(void);
#define nsec osnsec

/*
 * one-of-a-kind
 */


/* [standalone-contract:_assert]
 * EN: _assert: posix/fatal implementation and native state.
 * RU: _assert: реализация posix/fatal и нативное состояние.
 * message (char *message): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void _assert(char *);


/* [standalone-contract:charstod]
 * EN: charstod: original POSIX lib9 charstod support.
 * RU: charstod: исходная POSIX-поддержка lib9 charstod.
 * int (*f)(void *) (int (*f)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * vp (void *vp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern double charstod(int (*)(void *), void *);


/* [standalone-contract:cleanname]
 * EN: cleanname: lexical path normalization without filesystem lookup.
 * RU: cleanname: лексическая нормализация пути без обращения к файловой системе.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *cleanname(char *);


/* [standalone-contract:getcallerpc]
 * EN: getcallerpc: original POSIX lib9 getcallerpc-FreeRTOS-riscv64 support.
 * RU: getcallerpc: исходная POSIX-поддержка lib9 getcallerpc-FreeRTOS-riscv64.
 * x (void *x): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern uintptr getcallerpc(void *);


/* [standalone-contract:getfields]
 * EN: getfields: original POSIX lib9 getfields support.
 * RU: getfields: исходная POSIX-поддержка lib9 getfields.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (char **args): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * max (int max): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mflag (int mflag): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * set (char *set): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int getfields(char *, char **, int, int, char *);


/* [standalone-contract:getuser]
 * EN: getuser: original POSIX lib9 getuser-Nt support.
 * RU: getuser: исходная POSIX-поддержка lib9 getuser-Nt.
 * Parameters: none. Параметры: нет.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *getuser(void);


/* [standalone-contract:getwd]
 * EN: getwd: original POSIX lib9 getwd-Nt support.
 * RU: getwd: исходная POSIX-поддержка lib9 getwd-Nt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: char *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern char *getwd(char *, int);
extern double ipow10(int);
#define pow10 infpow10


/* [standalone-contract:pow10]
 * EN: pow10: original POSIX lib9 pow10 support.
 * RU: pow10: исходная POSIX-поддержка lib9 pow10.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern double pow10(int);


/* [standalone-contract:strtoll]
 * EN: strtoll: original POSIX lib9 strtoll support.
 * RU: strtoll: исходная POSIX-поддержка lib9 strtoll.
 * nptr (const char *nptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * endptr (char **endptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * base (int base): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: vlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: vlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern vlong strtoll(const char *, char **, int);
#define qsort infqsort
