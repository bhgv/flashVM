/* Copyright (c) 2026 bhgv
 * EN: Native server transport staging, original-core hooks and independently retained wake context.
 * RU: Native server transport staging, original-core hooks и independently retained wake context.
 */
#ifndef STYX_PHY_INTERNAL_H
#define STYX_PHY_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "packet.h"
#include <stdint.h>
#include <time.h>
#include <errno.h>

/* EN/RU: Per-connection logical fid state only; original server independently owns all authoritative fid/Qid storage.
 * Только logical fid state соединения; original server независимо владеет authoritative fid/Qid storage.
 */
typedef struct StyxWalkFid
{
    u32int id;     /* Original protocol fid number, not a native pointer. */
    int directory; /* Last acknowledged Qid kind. */
    uvlong
        node;   /* Acknowledged held-object Qid path; names alone do not identify an opened object after replacement. */
    int opened; /* Topen acknowledged; Tstat then describes the held object rather than named metadata. */
    char path[WALK_PATH_LIMIT]; /* Logical location within the fixed exported root, used only for traversal identity. */
    struct StyxWalkFid *next;   /* Owned bounded metadata chain. */
} StyxWalkFid;

/* EN/RU: Owner-only I/O state; only references and immutable PHY wake access cross threads.
 * Owner-only I/O state; между threads только references и immutable PHY wake access. */
struct StyxPhyWake
{
    int walk_mode;          /* Explicit negotiated history envelope, never inferred from ordinary traffic. */
    unsigned long walk_tag; /* Data purpose assigned by the peer in tag0, acknowledged on every reply. */
    WalkEntity walk_root;   /* Trusted fixed granted-root identity, not a host path or credential. */
    int walk_cycle; /* Explicit current callback failure kind; unrelated skipped cycles never annotate denials. */
    unsigned long walk_before; /* Native cycle serial at this original-core dispatch entry. */
    StyxWalkFid *walk_fids; /* Owned scalar active-fid metadata, bounded separately from original server authority. */
    StyxWalkFid *walk_pending;         /* Owned staged attach/walk candidate until acknowledged reply. */
    unsigned walk_fid_count;           /* Actual scalar tracker count. */
    unsigned char *walk_decode;        /* Owned separate original-request codec scratch in history mode. */
    Fcall walk_request;                /* Borrowed decoded pointers into walk_decode through this one dispatch. */
    char walk_base[WALK_PATH_LIMIT];   /* Copied original walk base, stable for in-place/partial acknowledgement. */
    char walk_target[WALK_PATH_LIMIT]; /* Immutable active endpoint remainder through dispatch. */
    unsigned capacity;                 /* Bounded incoming encoded-frame storage. */
    unsigned stride;                   /* Fixed outgoing slot extent, reply history is always empty. */
    RemotePhy *phy;                    /* Owned until the final owner/wake pin retires. */
    size_t references;                 /* Atomic server owner plus independent native wake pins. */
    uchar *receive;                    /* Owned one-frame receive buffer, MSGMAX bytes. */
    unsigned used;                     /* Received confirmed prefix. */
    unsigned wanted;                   /*4 for size header, then full original frame length. */
    int complete;                      /* One full frame may be handed to original server. */
    uchar *output;                     /* Owned bounded ring of queued_frames*MSGMAX bytes. */
    unsigned *lengths;                 /* Owned per-slot encoded extents. */
    unsigned slots;                    /* Fixed reply queue bound1..128. */
    unsigned first;                    /* Current reply slot. */
    unsigned queued;                   /* Queued complete reply slots. */
    unsigned sent;                     /* Confirmed prefix of current first slot. */
    int accepted;                      /* Single already connected peer admitted exactly once. */
    int processing;                    /* Reserve one reply slot while original core may generate a response. */
    int failed;                        /* Sticky carrier/framing/queue failure. */
};

extern __thread StyxPhyWake *styxphyowner; /* Current original-core call on this native owner only. */


/* EN/RU: Original core entry points renamed only in the inclusion translation unit.
 * Исходные entry points переименованы только в inclusion translation unit. */
char *styxcoreinit(Styxserver *, Styxops *, char *, int, int);
char *styxcoreprocess(Styxserver *);


/* [standalone-contract:styxendserver]
 * EN: styxendserver: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxendserver: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxendserver(Styxserver *);


/* EN/RU: Owner pumps bounded nonblocking receive/send; callbacks never enter a VM.
 * Owner выполняет bounded nonblocking receive/send; callbacks без VM entry. */


/* [standalone-contract:styxphyreceive]
 * EN: styxphyreceive: phy/io implementation and native state.
 * RU: styxphyreceive: реализация phy/io и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphyreceive(StyxPhyWake *context);

/* EN/RU: Shared explicit raw/history constructor; consumes PHY always, copies optional root metadata. */


/* [standalone-contract:styxphyinitprofile]
 * EN: styxphyinitprofile: phy/lifetime implementation and native state.
 * RU: styxphyinitprofile: реализация phy/lifetime и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * ops (Styxops *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * permissions (int permissions): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * needfile (int needfile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * queued_frames (unsigned queued_frames): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * root (const WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxphyinitprofile(Styxserver *, Styxops *, RemotePhy *, int, int, unsigned, const WalkEntity *);

/* EN/RU: Import a complete control/data request before raw dispatch,1/0; caller leaves successful scope. */


/* [standalone-contract:styxphywalkenter]
 * EN: styxphywalkenter: phy/traversal implementation and native state.
 * RU: styxphywalkenter: реализация phy/traversal и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * scope (WalkScope *scope): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkenter(StyxPhyWake *, WalkScope *);

/* EN/RU: Refuse a proven repeated endpoint without executing original side effects;1/0 enqueue result. */


/* [standalone-contract:styxphywalkreject]
 * EN: styxphywalkreject: phy/traversal implementation and native state.
 * RU: styxphywalkreject: реализация phy/traversal и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkreject(Styxserver *);

/* EN/RU: Original backend reply enqueue; borrowed bytes/count, return accepted raw extent/-1. */


/* [standalone-contract:styxsend]
 * EN: styxsend: phy/io implementation and native state.
 * RU: styxsend: реализация phy/io и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * token (int token): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * bytes (char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * flags (int flags): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxsend(Styxserver *, int, char *, int, int);

/* EN/RU: Restore endpoint request scope, apply only acknowledged fid metadata, and free tracker owners. */


/* [standalone-contract:styxphywalkrequest]
 * EN: styxphywalkrequest: phy/traversal_fids implementation and native state.
 * RU: styxphywalkrequest: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame (const void *frame): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * endpoint (WalkScope *endpoint): Request-local active traversal guard; successful entry requires its matching leave.
 * RU: Локальный guard активного обхода запроса; успешный enter требует соответствующего leave.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkrequest(StyxPhyWake *, const void *, unsigned, WalkScope *);


/* [standalone-contract:styxphywalkresponse]
 * EN: styxphywalkresponse: phy/traversal_fids implementation and native state.
 * RU: styxphywalkresponse: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame (const void *frame): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * length (unsigned length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxphywalkresponse(StyxPhyWake *, const void *, unsigned);


/* [standalone-contract:styxphywalkclear]
 * EN: styxphywalkclear: phy/traversal_fids implementation and native state.
 * RU: styxphywalkclear: реализация phy/traversal_fids и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphywalkclear(StyxPhyWake *);


/* [standalone-contract:styxphyoutput]
 * EN: styxphyoutput: phy/io implementation and native state.
 * RU: styxphyoutput: реализация phy/io и нативное состояние.
 * context (StyxPhyWake *context): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxphyoutput(StyxPhyWake *context);


/* EN/RU: Native monotonic milliseconds, negative on clock failure; no carrier or guest calls.
 * Native monotonic milliseconds, negative при clock failure; без carrier/guest calls. */


/* [standalone-contract:styxphytime]
 * EN: styxphytime: phy/io implementation and native state.
 * RU: styxphytime: реализация phy/io и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: double; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
double styxphytime(void);
#endif
