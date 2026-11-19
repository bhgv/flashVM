/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of lib9.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

/* [standalone-contract:qsort]
 * EN: qsort: original POSIX lib9 qsort support.
 * RU: qsort: исходная POSIX-поддержка lib9 qsort.
 * va (void *va): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * es (long es): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * int (*cmp)(void *, void *) (int (*cmp)(void *, void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void qsort(void *, long, long, int (*)(void *, void *));


/* [standalone-contract:strtoull]
 * EN: strtoull: original POSIX lib9 strtoull support.
 * RU: strtoull: исходная POSIX-поддержка lib9 strtoull.
 * nptr (const char *nptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * endptr (char **endptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * base (int base): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: uvlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uvlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern uvlong strtoull(const char *, char **, int);


/* [standalone-contract:sysfatal]
 * EN: sysfatal: original POSIX lib9 sysfatal support.
 * RU: sysfatal: исходная POSIX-поддержка lib9 sysfatal.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void sysfatal(char *, ...);


/* [standalone-contract:dec64]
 * EN: dec64: original POSIX lib9 u64 support.
 * RU: dec64: исходная POSIX-поддержка lib9 u64.
 * out (uchar *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dec64(uchar *, int, char *, int);


/* [standalone-contract:enc64]
 * EN: enc64: original POSIX lib9 u64 support.
 * RU: enc64: исходная POSIX-поддержка lib9 u64.
 * out (char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (uchar *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int enc64(char *, int, uchar *, int);


/* [standalone-contract:dec32]
 * EN: dec32: original POSIX lib9 u32 support.
 * RU: dec32: исходная POSIX-поддержка lib9 u32.
 * dest (uchar *dest): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ndest (int ndest): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * src (char *src): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nsrc (int nsrc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dec32(uchar *, int, char *, int);


/* [standalone-contract:enc32]
 * EN: enc32: original POSIX lib9 u32 support.
 * RU: enc32: исходная POSIX-поддержка lib9 u32.
 * dest (char *dest): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ndest (int ndest): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * src (uchar *src): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nsrc (int nsrc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int enc32(char *, int, uchar *, int);


/* [standalone-contract:dec16]
 * EN: dec16: original POSIX lib9 u16 support.
 * RU: dec16: исходная POSIX-поддержка lib9 u16.
 * out (uchar *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dec16(uchar *, int, char *, int);


/* [standalone-contract:enc16]
 * EN: enc16: original POSIX lib9 u16 support.
 * RU: enc16: исходная POSIX-поддержка lib9 u16.
 * out (char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (uchar *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int enc16(char *, int, uchar *, int);
extern int encodefmt(Fmt *);

/*
 *  synchronization
 */
typedef struct Lock
{
    int val;
    int pid;
} Lock;

/* [standalone-contract:_tas]
 * EN: _tas: original POSIX lib9 lock-Nt-386 support.
 * RU: _tas: исходная POSIX-поддержка lib9 lock-Nt-386.
 * la (int *la): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int _tas(int *);


/* [standalone-contract:lock]
 * EN: lock: original POSIX lib9 lock support.
 * RU: lock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void lock(Lock *);


/* [standalone-contract:unlock]
 * EN: unlock: original POSIX lib9 lock support.
 * RU: unlock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void unlock(Lock *);


/* [standalone-contract:canlock]
 * EN: canlock: original POSIX lib9 lock support.
 * RU: canlock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int canlock(Lock *);

typedef struct QLock QLock;

struct QLock
{
    Lock use;   /* to access Qlock structure */
    Proc *head; /* next process waiting for object */
    Proc *tail; /* last process waiting for object */
    int locked; /* flag */
};

/* [standalone-contract:qlock]
 * EN: qlock: original POSIX lib9 lock support.
 * RU: qlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void qlock(QLock *);


/* [standalone-contract:qunlock]
 * EN: qunlock: original POSIX lib9 lock support.
 * RU: qunlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void qunlock(QLock *);


/* [standalone-contract:canqlock]
 * EN: canqlock: original POSIX lib9 lock support.
 * RU: canqlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int canqlock(QLock *);
extern void _qlockinit(ulong (*)(ulong, ulong)); /* called only by the thread library */

typedef struct RWLock
{
    Lock l;      /* Lock modify lock */
    QLock x;     /* Mutual exclusion lock */
    QLock k;     /* Lock for waiting writers */
    int readers; /* Count of readers in lock */
} RWLock;

/* [standalone-contract:canrlock]
 * EN: canrlock: original POSIX lib9 lock support.
 * RU: canrlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int canrlock(RWLock *);
extern int canwlock(RWLock *);


/* [standalone-contract:rlock]
 * EN: rlock: original POSIX lib9 lock support.
 * RU: rlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void rlock(RWLock *);


/* [standalone-contract:runlock]
 * EN: runlock: original POSIX lib9 lock support.
 * RU: runlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void runlock(RWLock *);


/* [standalone-contract:wlock]
 * EN: wlock: original POSIX lib9 lock support.
 * RU: wlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void wlock(RWLock *);


/* [standalone-contract:wunlock]
 * EN: wunlock: original POSIX lib9 lock support.
 * RU: wunlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void wunlock(RWLock *);

/*
 * network dialing
 */
#define NETPATHLEN 40

/*
 * system calls
 *
 */
#define STATMAX 65535U                 /* max length of machine-independent stat structure */
#define DIRMAX (sizeof(Dir) + STATMAX) /* max length of Dir structure */
#define ERRMAX 128                     /* max length of error string */

#define MORDER 0x0003                  /* mask for bits defining order of mounting */
#define MREPL 0x0000                   /* mount replaces object */
#define MBEFORE 0x0001                 /* mount goes before others in union directory */
#define MAFTER 0x0002                  /* mount goes after others in union directory */
#define MCREATE 0x0004                 /* permit creation in mounted directory */
#define MCACHE 0x0010                  /* cache some data */
#define MMASK 0x0017                   /* all bits on */

#define OREAD 0                        /* open for read */
#define OWRITE 1                       /* write */
#define ORDWR 2                        /* read and write */
#define OEXEC 3                        /* execute, == read but check execute permission */
#define OTRUNC 16                      /* or'ed in (except for exec), truncate file first */
#define OCEXEC 32                      /* or'ed in, close on exec */
#define ORCLOSE 64                     /* or'ed in, remove on close */
#define OEXCL 0x1000                   /* or'ed in, exclusive use (create only) */

#define AEXIST 0                       /* accessible: exists */
#define AEXEC 1                        /* execute access */
#define AWRITE 2                       /* write access */
#define AREAD 4                        /* read access */

/* bits in Qid.type */
#define QTDIR 0x80    /* type bit for directories */
#define QTAPPEND 0x40 /* type bit for append only files */
#define QTEXCL 0x20   /* type bit for exclusive use files */
#define QTMOUNT 0x10  /* type bit for mounted channel */
#define QTAUTH 0x08   /* type bit for authentication file */
#define QTFILE 0x00   /* plain file */

/* bits in Dir.mode */
#define DMDIR 0x80000000    /* mode bit for directories */
#define DMAPPEND 0x40000000 /* mode bit for append only files */
#define DMEXCL 0x20000000   /* mode bit for exclusive use files */
#define DMMOUNT 0x10000000  /* mode bit for mounted channel */
#define DMAUTH 0x08000000   /* mode bit for authentication file */
#define DMREAD 0x4          /* mode bit for read permission */
#define DMWRITE 0x2         /* mode bit for write permission */
#define DMEXEC 0x1          /* mode bit for execute permission */

typedef struct Qid
{
    uvlong path;
    ulong vers;
    uchar type;
} Qid;

typedef struct Dir
{
    /* system-modified data */
    ushort type; /* server type */
    uint dev;    /* server subtype */

                 /* file data */
    Qid qid;      /* unique id from server */
    ulong mode;   /* permissions */
    ulong atime;  /* last read time */
    ulong mtime;  /* last write time */
    vlong length; /* file length */
    char *name;   /* last element of path */
    char *uid;    /* owner name */
    char *gid;    /* group name */
    char *muid;   /* last modifier name */
} Dir;
