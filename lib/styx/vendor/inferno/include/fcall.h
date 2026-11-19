/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: include/fcall implementation and native state.
 * RU: реализация include/fcall и нативное состояние.
 */
#pragma src "/usr/inferno/lib9"
#pragma lib "libc.a"

#define VERSION9P "9P2000"

#define MAXWELEM 16

typedef struct Fcall
{
    uchar type;
    u32int fid;
    ushort tag;
    /* union { */
    /* struct { */
    u32int msize;          /* Tversion, Rversion */
    char *version;         /* Tversion, Rversion */
                           /* }; */
                           /* struct { */
    ushort oldtag;         /* Tflush */
                           /* }; */
                           /* struct { */
    char *ename;           /* Rerror */
                           /* }; */
                           /* struct { */
    Qid qid;               /* Rattach, Ropen, Rcreate */
    u32int iounit;         /* Ropen, Rcreate */
                           /* }; */
                           /* struct { */
    Qid aqid;              /* Rauth */
                           /* }; */
                           /* struct { */
    u32int afid;           /* Tauth, Tattach */
    char *uname;           /* Tauth, Tattach */
    char *aname;           /* Tauth, Tattach */
                           /* }; */
                           /* struct { */
    u32int perm;           /* Tcreate */
    char *name;            /* Tcreate */
    uchar mode;            /* Tcreate, Topen */
                           /* }; */
                           /* struct { */
    u32int newfid;         /* Twalk */
    ushort nwname;         /* Twalk */
    char *wname[MAXWELEM]; /* Twalk */
                           /* }; */
                           /* struct { */
    ushort nwqid;          /* Rwalk */
    Qid wqid[MAXWELEM];    /* Rwalk */
                           /* }; */
                           /* struct { */
    vlong offset;          /* Tread, Twrite */
    u32int count;          /* Tread, Twrite, Rread */
    char *data;            /* Twrite, Rread */
                           /* }; */
                           /* struct { */
    ushort nstat;          /* Twstat, Rstat */
    uchar *stat;           /* Twstat, Rstat */
                           /* }; */
                           /* }; */
} Fcall;

#define GBIT8(p) ((p)[0])
#define GBIT16(p) ((p)[0] | ((p)[1] << 8))
/* Assemble wire integers with unsigned shifts, including each high byte. */
#define GBIT32(p) ((u32int)(p)[0] | ((u32int)(p)[1] << 8) | ((u32int)(p)[2] << 16) | ((u32int)(p)[3] << 24))
#define GBIT64(p) ((uvlong)GBIT32(p) | ((uvlong)GBIT32((p) + 4) << 32))

#define PBIT8(p, v) (p)[0] = (v)
#define PBIT16(p, v)                                                                                                   \
    (p)[0] = (v);                                                                                                      \
    (p)[1] = (v) >> 8
#define PBIT32(p, v)                                                                                                   \
    (p)[0] = (v);                                                                                                      \
    (p)[1] = (v) >> 8;                                                                                                 \
    (p)[2] = (v) >> 16;                                                                                                \
    (p)[3] = (v) >> 24
#define PBIT64(p, v)                                                                                                   \
    (p)[0] = (v);                                                                                                      \
    (p)[1] = (v) >> 8;                                                                                                 \
    (p)[2] = (v) >> 16;                                                                                                \
    (p)[3] = (v) >> 24;                                                                                                \
    (p)[4] = (v) >> 32;                                                                                                \
    (p)[5] = (v) >> 40;                                                                                                \
    (p)[6] = (v) >> 48;                                                                                                \
    (p)[7] = (v) >> 56

#define BIT8SZ 1
#define BIT16SZ 2
#define BIT32SZ 4
#define BIT64SZ 8
#define QIDSZ (BIT8SZ + BIT32SZ + BIT64SZ)

/* STATFIXLEN includes leading 16-bit count */
/* The count, however, excludes itself; total size is BIT16SZ+count */
#define STATFIXLEN                                                                                                     \
    (BIT16SZ + QIDSZ + 5 * BIT16SZ + 4 * BIT32SZ + 1 * BIT64SZ) /* amount of fixed length data in a stat buffer */

#define NOTAG (ushort) ~0U                                      /* Dummy tag */
#define NOFID (u32int) ~0U                                      /* Dummy fid */
#define IOHDRSZ 24                                              /* ample room for Twrite/Rread header (iounit) */

enum
{
    Tversion = 100,
    Rversion,
    Tauth = 102,
    Rauth,
    Tattach = 104,
    Rattach,
    Terror = 106, /* illegal */
    Rerror,
    Tflush = 108,
    Rflush,
    Twalk = 110,
    Rwalk,
    Topen = 112,
    Ropen,
    Tcreate = 114,
    Rcreate,
    Tread = 116,
    Rread,
    Twrite = 118,
    Rwrite,
    Tclunk = 120,
    Rclunk,
    Tremove = 122,
    Rremove,
    Tstat = 124,
    Rstat,
    Twstat = 126,
    Rwstat,
    Tmax,
};

/* [standalone-contract:convM2S]
 * EN: Decode one original 9P2000 message, validating counted fields inside its declared extent.
 * RU: Декодировать исходное 9P2000-сообщение с проверкой counted fields внутри объявленной длины.
 * ap (uchar *ap): Mutable input message; decoded string/data pointers borrow this storage.
 * RU: Изменяемое входное сообщение; строки/data результата заимствуют эту память.
 * nap (uint nap): Accessible input bytes, including the four-byte message length.
 * RU: Доступные входные байты, включая четырёхбайтовую длину сообщения.
 * f (Fcall *f): Output Fcall storage. On zero return, discard its potentially partial fields.
 * RU: Приёмник Fcall. При возврате нуля частично заполненные поля отбрасываются.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convM2S(uchar *, uint, Fcall *);


/* [standalone-contract:convS2M]
 * EN: Encode a native Fcall as standard 9P2000 bytes with explicit little-endian fields.
 * RU: Кодировать native Fcall в стандартные 9P2000 байты с явным little-endian полей.
 * f (Fcall *f): Input message fields and borrowed strings/data to encode; caller retains ownership.
 * RU: Входные поля и заимствованные строки/data; владение остаётся у вызывающего.
 * ap (uchar *ap): Caller-owned output storage, disjoint from referenced input fields.
 * RU: Выходной буфер вызывающего, не перекрывающий входные поля.
 * nap (uint nap): Available output bytes; sizeS2M reports the required complete extent.
 * RU: Доступная выходная ёмкость; sizeS2M сообщает размер целого сообщения.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convS2M(Fcall *, uchar *, uint);


/* [standalone-contract:sizeS2M]
 * EN: Calculate the exact standard message size before encoding; reject unsupported or unrepresentable fields.
 * RU: Вычислить точный размер стандартного сообщения до кодирования; отклонить неподдерживаемые поля.
 * f (Fcall *f): Input message to measure, with valid referenced strings and payload.
 * RU: Входное сообщение для измерения с допустимыми строками и payload.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint sizeS2M(Fcall *);


/* [standalone-contract:statcheck]
 * EN: Validate a complete encoded stat record and each counted string without exposing a partial record.
 * RU: Проверить целую stat-запись и каждую counted string, не выдавая частично разобранную запись.
 * buf (uchar *buf): Counted stat record to validate structurally.
 * RU: Counted stat-запись для структурной проверки.
 * nbuf (uint nbuf): Accessible extent; must contain the complete counted record.
 * RU: Доступная длина; должна содержать целую counted запись.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int statcheck(uchar *abuf, uint nbuf);


/* [standalone-contract:convM2D]
 * EN: Decode a validated standard Dir record and copy its strings into caller-supplied storage when provided.
 * RU: Декодировать проверенную Dir-запись и скопировать строки в буфер вызывающего, если он задан.
 * buf (uchar *buf): Input counted stat bytes; no native Dir layout is accepted.
 * RU: Входные counted stat-байты; native layout Dir не принимается.
 * nbuf (uint nbuf): Accessible input extent in bytes.
 * RU: Доступная входная длина в байтах.
 * d (Dir *d): Caller-owned decoded scalar fields and string pointers.
 * RU: Приёмник декодированных scalar fields и указателей строк.
 * strs (char *strs): Caller-owned decoded-string storage, or NULL for empty string fields. nbuf bytes suffice for a valid record.
 * RU: Буфер декодированных строк либо NULL для пустых string fields. Для допустимой записи достаточно nbuf байт.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convM2D(uchar *, uint, Dir *, char *);


/* [standalone-contract:convD2M]
 * EN: Encode one standard Dir record; a short buffer can receive only its two-byte required-size prefix.
 * RU: Кодировать стандартную Dir-запись; короткий буфер может получить только двухбайтовый prefix требуемой длины.
 * d (Dir *d): Input directory metadata to encode as one counted stat record.
 * RU: Входные metadata для одной counted stat-записи.
 * buf (uchar *buf): Caller-owned writable destination; do not confuse a two-byte size prefix with a complete record.
 * RU: Выходной буфер вызывающего; двухбайтовый size prefix не является целой записью.
 * nbuf (uint nbuf): Destination capacity in bytes, including the leading two-byte length.
 * RU: Ёмкость приёмника в байтах, включая двухбайтовую длину.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convD2M(Dir *, uchar *, uint);


/* [standalone-contract:sizeD2M]
 * EN: Calculate the complete standard stat-record size, including its leading size field.
 * RU: Вычислить полный размер стандартной stat-записи, включая начальное поле длины.
 * d (Dir *d): Input directory metadata; strings must be valid NUL-terminated C strings.
 * RU: Входные metadata каталога; строки должны быть допустимыми C-строками с NUL.
 * Return: uint; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint sizeD2M(Dir *);


/* [standalone-contract:fcallfmt]
 * EN: fcallfmt: original POSIX lib9 fcallfmt support.
 * RU: fcallfmt: исходная POSIX-поддержка lib9 fcallfmt.
 * fmt (Fmt *fmt): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fcallfmt(Fmt *);


/* [standalone-contract:dirfmt]
 * EN: dirfmt: original POSIX lib9 fcallfmt support.
 * RU: dirfmt: исходная POSIX-поддержка lib9 fcallfmt.
 * fmt (Fmt *fmt): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dirfmt(Fmt *);
int dirmodefmt(Fmt *);

int read9pmsg(int, void *, uint);

#pragma varargck type "F" Fcall *
#pragma varargck type "M" ulong
#pragma varargck type "D" Dir *
