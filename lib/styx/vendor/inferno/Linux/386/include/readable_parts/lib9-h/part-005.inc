/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of lib9.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* [standalone-contract:dirstat]
 * EN: dirstat: original POSIX lib9 dirstat-Nt support.
 * RU: dirstat: исходная POSIX-поддержка lib9 dirstat-Nt.
 * f (char *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Dir *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Dir *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Dir *dirstat(char *);


/* [standalone-contract:dirfstat]
 * EN: dirfstat: original POSIX lib9 dirstat-Nt support.
 * RU: dirfstat: исходная POSIX-поддержка lib9 dirstat-Nt.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Dir *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Dir *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern Dir *dirfstat(int);


/* [standalone-contract:dirwstat]
 * EN: dirwstat: original POSIX lib9 dirwstat support.
 * RU: dirwstat: исходная POSIX-поддержка lib9 dirwstat.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dirwstat(char *, Dir *);


/* [standalone-contract:dirfwstat]
 * EN: dirfwstat: original POSIX lib9 dirwstat support.
 * RU: dirfwstat: исходная POSIX-поддержка lib9 dirwstat.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int dirfwstat(int, Dir *);
extern long dirread(int, Dir **);


/* [standalone-contract:nulldir]
 * EN: nulldir: original POSIX lib9 nulldir support.
 * RU: nulldir: исходная POSIX-поддержка lib9 nulldir.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void nulldir(Dir *);
extern long dirreadall(int, Dir **);

typedef struct Waitmsg
{
    int pid;       /* of loved one */
    ulong time[3]; /* of loved one & descendants */
    char *msg;
} Waitmsg;

/* [standalone-contract:_exits]
 * EN: _exits: original POSIX lib9 exits support.
 * RU: _exits: исходная POSIX-поддержка lib9 exits.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void _exits(char *);


/* [standalone-contract:exits]
 * EN: exits: original POSIX lib9 exits support.
 * RU: exits: исходная POSIX-поддержка lib9 exits.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void exits(char *);


/* [standalone-contract:create]
 * EN: create: original POSIX lib9 create support.
 * RU: create: исходная POSIX-поддержка lib9 create.
 * f (char *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * perm (int perm): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int create(char *, int, int);


/* [standalone-contract:errstr]
 * EN: errstr: original POSIX lib9 errstr-Nt support.
 * RU: errstr: исходная POSIX-поддержка lib9 errstr-Nt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nerr (uint nerr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern int errstr(char *, uint);

extern void perror(const char *);


/* [standalone-contract:readn]
 * EN: readn: original POSIX lib9 readn support.
 * RU: readn: исходная POSIX-поддержка lib9 readn.
 * f (int f): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * av (void *av): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern long readn(int, void *, long);
extern int remove(const char *);


/* [standalone-contract:rerrstr]
 * EN: rerrstr: original POSIX lib9 rerrstr support.
 * RU: rerrstr: исходная POSIX-поддержка lib9 rerrstr.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nbuf (uint nbuf): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void rerrstr(char *, uint);


/* [standalone-contract:seek]
 * EN: seek: original POSIX lib9 seek support.
 * RU: seek: исходная POSIX-поддержка lib9 seek.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * where (vlong where): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * from (int from): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: vlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: vlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern vlong seek(int, vlong, int);
extern int segflush(void *, ulong);


/* [standalone-contract:werrstr]
 * EN: werrstr: original POSIX lib9 errstr-Nt support.
 * RU: werrstr: исходная POSIX-поддержка lib9 errstr-Nt.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void werrstr(char *, ...);

extern char *argv0;
#define ARGBEGIN                                                                                                       \
    for ((argv0 || (argv0 = *argv)), argv++, argc--; argv[0] && argv[0][0] == '-' && argv[0][1]; argc--, argv++)       \
    {                                                                                                                  \
        char *_args, *_argt;                                                                                           \
        Rune _argc;                                                                                                    \
        _args = &argv[0][1];                                                                                           \
        if (_args[0] == '-' && _args[1] == 0)                                                                          \
        {                                                                                                              \
            argc--;                                                                                                    \
            argv++;                                                                                                    \
            break;                                                                                                     \
        }                                                                                                              \
        _argc = 0;                                                                                                     \
        while (*_args && (_args += chartorune(&_argc, _args)))                                                         \
            switch (_argc)
#define ARGEND                                                                                                         \
    SET(_argt);                                                                                                        \
    USED(_argt);                                                                                                       \
    USED(_argc);                                                                                                       \
    USED(_args);                                                                                                       \
    }                                                                                                                  \
    USED(argv);                                                                                                        \
    USED(argc);
#define ARGF() (_argt = _args, _args = "", (*_argt ? _argt : argv[1] ? (argc--, *++argv) : 0))
#define EARGF(x) (_argt = _args, _args = "", (*_argt ? _argt : argv[1] ? (argc--, *++argv) : ((x), abort(), (char *)0)))

#define ARGC() _argc

/*
 *	Extensions for Inferno to basic libc.h
 */

#define setbinmode()

/* FCR */
#define FPINEX (1 << 5)
#define FPUNFL ((1 << 4) | (1 << 1))
#define FPOVFL (1 << 3)
#define FPZDIV (1 << 2)
#define FPINVAL (1 << 0)
#define FPRNR (0 << 10)
#define FPRZ (3 << 10)
#define FPRPINF (2 << 10)
#define FPRNINF (1 << 10)
#define FPRMASK (3 << 10)
#define FPPEXT (3 << 8)
#define FPPSGL (0 << 8)
#define FPPDBL (2 << 8)
#define FPPMASK (3 << 8)
/* FSR */
#define FPAINEX FPINEX
#define FPAOVFL FPOVFL
#define FPAUNFL FPUNFL
#define FPAZDIV FPZDIV
#define FPAINVAL FPINVAL


/* [standalone-contract:setfcr]
 * EN: setfcr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.
 * RU: setfcr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.
 * fcr (ulong fcr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void setfcr(ulong);


/* [standalone-contract:setfsr]
 * EN: setfsr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.
 * RU: setfsr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.
 * fsr (ulong fsr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern void setfsr(ulong);


/* [standalone-contract:getfcr]
 * EN: getfcr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.
 * RU: getfcr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong getfcr(void);


/* [standalone-contract:getfsr]
 * EN: getfsr: original POSIX lib9 setfcr-FreeRTOS-riscv64 support.
 * RU: getfsr: исходная POSIX-поддержка lib9 setfcr-FreeRTOS-riscv64.
 * Parameters: none. Параметры: нет.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
extern ulong getfsr(void);
