/* Copyright (c) 2026 bhgv
 * EN: Carrier-independent native transport capabilities, precise byte I/O and interruptible readiness.
 * RU: Независимые от носителя native transport capabilities, точное byte I/O и interruptible readiness.
 */
#ifndef REMOTE_PHY_H
#define REMOTE_PHY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct RemotePhy RemotePhy; /* Owned native carrier, never a VM or wire pointer. */
#define PHY_OK 0                    /* Operation succeeded, count is authoritative only with PREFIX. */
#define PHY_AGAIN 1                 /* Temporary lack of progress, never EOF. */
#define PHY_EOF 2                   /* Read direction ended, count must be zero. */
#define PHY_ERROR (-1)              /* Permanent I/O or malformed-driver result. */
#define PHY_UNSUPPORTED (-2)        /* Driver does not support the requested optional operation. */
#define PHY_PREFIX 1U               /* Count is the confirmed leading extent. */
#define PHY_UNCERTAIN 2U            /* Effects beyond prefix unknown; never replay automatically. */
#define PHY_READ 1U                 /* Wait for read readiness. */
#define PHY_WRITE 2U                /* Wait for write readiness. */
#define PHY_WAKE 4U                 /* Local native work/cancellation interrupted the wait. */
#define PHY_HANGUP 8U               /* Drain readable prefix, then observe EOF/error. */
#define PHY_FAILED 16U              /* Readiness source failed. */
#define PHY_STREAM 1U               /* Driver exposes a byte stream, not packet boundaries. */
#define PHY_ORDERED 2U              /* Bytes retain order. */
#define PHY_RELIABLE 4U       /* Loss/duplication/corruption handled below this interface or reported as failure. */
#define PHY_HALF_DUPLEX 8U    /* Driver arbitrates turn-taking below logical byte I/O. */
#define PHY_BIDIR_SESSION 32U /* Logical stream from an explicitly configured bidirectional session adapter. */
#define PHY_PACKET 16U        /* Raw message carrier; requires a suitable adaptation before 9P. */

/* EN/RU: Copied native acknowledgement; not a public protocol structure.
 * Native acknowledgement, не public protocol structure. */
typedef struct PhyTransfer
{
    size_t count;   /* Confirmed prefix only. */
    int status;     /* OK/AGAIN/EOF/ERROR; UNSUPPORTED only for optional operations. */
    unsigned flags; /* PREFIX and optional UNCERTAIN. */
} PhyTransfer;

/* EN/RU: Driver capabilities copied at creation; connected is local lifecycle, never a peer permission.
 * Driver capabilities copied при creation; connected — local lifecycle, не peer permission. */
typedef struct PhyInfo
{
    unsigned capabilities; /* Explicit STREAM/ORDERED/RELIABLE or PACKET and optional HALF_DUPLEX. */
    size_t mtu;            /* Physical message hint,0 unknown/not applicable. */
    size_t max_read;       /* Maximum callback request,0 no additional limit. */
    size_t max_write;      /* Maximum callback request,0 no additional limit. */
    int connected;         /* Set by successful connect/adoption, cleared by disconnect. */
} PhyInfo;

/* EN/RU: Native driver callbacks; read/write never block and borrow buffers until return.
 * Wait may block up to timeout ms (-1 infinite); wake must interrupt it, even before wait starts.
 * One owner serializes all except wake, which is thread-safe/nonwaiting until destroy.
 * connect parameters are borrowed driver-specific native data, not a URL or authentication.
 * destroy consumes state after I/O/wake callers retire; state adopted only on successful phycreate.
 * Native callbacks: read/write nonblocking; wait interruptible через sticky wake, один I/O owner.
 * connect получает borrowed native parameters; destroy после users, без guest/VM callbacks. */
typedef struct PhyOps
{
    PhyTransfer (*read)(void *, void *, size_t);        /* Nonblocking read, explicit EOF. */
    PhyTransfer (*write)(void *, const void *, size_t); /* Nonblocking precise write. */
    int (*wait)(void *, unsigned, int, unsigned *);     /* Requested readiness or WAKE/HANGUP/FAILED. */
    void (*wake)(void *);                               /* Nonwaiting sticky local readiness, no wire message. */
    int (*available)(void *, size_t *);                 /* Optional available-byte hint, UNSUPPORTED if unknown. */
    int (*connect)(void *, const void *, size_t);       /* Optional native connection setup. */
    void (*disconnect)(void *);                         /* Idempotent wire close; wake remains usable until destroy. */
    void (*destroy)(void *);                            /* Consume native driver storage, no protocol/guest wait. */
} PhyOps;

/* EN/RU: Borrow ops/info, adopt state only on owned success; NULL invalid/OOM.
 * Borrow ops/info; state принят только при success, NULL invalid/OOM. */


/* [standalone-contract:phycreate]
 * EN: phycreate: remote_phy/phy implementation and native state.
 * RU: phycreate: реализация remote_phy/phy и нативное состояние.
 * ops (const PhyOps *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * info (const PhyInfo *info): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: RemotePhy *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: RemotePhy *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
RemotePhy *phycreate(const PhyOps *ops, void *state, const PhyInfo *info);


/* EN/RU: Copy local capabilities/lifecycle;1/0, owner-serialized, no driver/transport I/O.
 * Копировать local info;1/0, owner-serialized, без driver I/O. */


/* [standalone-contract:phyinfo]
 * EN: phyinfo: remote_phy/phy implementation and native state.
 * RU: phyinfo: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * out (PhyInfo *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int phyinfo(RemotePhy *phy, PhyInfo *out);


/* EN/RU: Owner-serialized nonblocking read/write, borrowed buffers; validate precise driver results.
 * Size0 succeeds without a callback; cap hints bound a single driver request.
 * Owner-serialized read/write, buffers borrowed; size0 без callback, cap limits учитываются. */


/* [standalone-contract:phyread]
 * EN: phyread: remote_phy/phy implementation and native state.
 * RU: phyread: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: PhyTransfer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: PhyTransfer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
PhyTransfer phyread(RemotePhy *phy, void *bytes, size_t size);


/* EN/RU: Owner-serialized precise write of borrowed bytes/size; no replay or splitting on failure.
 * Owner-serialized precise write borrowed bytes/size; без replay/splitting при failure. */


/* [standalone-contract:phywrite]
 * EN: phywrite: remote_phy/phy implementation and native state.
 * RU: phywrite: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: PhyTransfer; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: PhyTransfer; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
PhyTransfer phywrite(RemotePhy *phy, const void *bytes, size_t size);


/* EN/RU: Owner waits for requested READ/WRITE; timeout>=-1,0 polls, returns OK/ERROR and ready bits.
 * No OS descriptor in this contract; driver integrates its own bus/IRQ/event mechanism.
 * Owner ждёт READ/WRITE, timeout>=-1;0 poll; OK/ERROR и ready bits, без требования OS fd. */


/* [standalone-contract:phywait]
 * EN: phywait: remote_phy/phy implementation and native state.
 * RU: phywait: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * wanted (unsigned wanted): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * timeout (int timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * ready (unsigned *ready): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int phywait(RemotePhy *phy, unsigned wanted, int timeout, unsigned *ready);


/* EN/RU: Nonwaiting native interrupt while lifetime pinned; NULL harmless, no wire traffic.
 * Native interrupt под lifetime pin; NULL harmless, без wire traffic. */


/* [standalone-contract:phywake]
 * EN: phywake: remote_phy/phy implementation and native state.
 * RU: phywake: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void phywake(RemotePhy *phy);


/* EN/RU: Optional local byte hint, not EOF or permission; owner-serialized, output unchanged on failure.
 * Optional byte hint, не EOF/permission; owner-serialized, output сохраняется при failure. */


/* [standalone-contract:phyavailable]
 * EN: phyavailable: remote_phy/phy implementation and native state.
 * RU: phyavailable: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * bytes (size_t *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int phyavailable(RemotePhy *phy, size_t *bytes);


/* EN/RU: Owner connects with borrowed driver-native parameters; OK updates local connected state.
 * Live carrier accepts only empty idempotent connect, not reconfiguration.
 * Owner connect с borrowed driver-native parameters; OK обновляет connected. */


/* [standalone-contract:phyconnect]
 * EN: phyconnect: remote_phy/phy implementation and native state.
 * RU: phyconnect: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * parameters (const void *parameters): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int phyconnect(RemotePhy *phy, const void *parameters, size_t size);


/* EN/RU: Owner disconnects once; destroy/free only after every I/O/wake user retires.
 * Owner disconnect один раз; free после всех I/O/wake users. */


/* [standalone-contract:phydisconnect]
 * EN: phydisconnect: remote_phy/phy implementation and native state.
 * RU: phydisconnect: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void phydisconnect(RemotePhy *phy);


/* EN/RU: Consume owned carrier after all I/O/wake callers retire; NULL harmless.
 * Поглотить owned carrier после всех I/O/wake callers; NULL harmless. */


/* [standalone-contract:phyfree]
 * EN: phyfree: remote_phy/phy implementation and native state.
 * RU: phyfree: реализация remote_phy/phy и нативное состояние.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void phyfree(RemotePhy *phy);

/* EN/RU: Host-selected factories; no global registry, auto-loading or guessed network authority.
 * Host-selected factories без global registry/auto-loading/authority по имени. */
typedef struct PhyDriver
{
    const char *name;                         /* Borrowed immutable selector, caller-defined namespace. */
    RemotePhy *(*open)(const void *, size_t); /* Borrow parameters; handle ownership defined by driver. */
} PhyDriver;

/* EN/RU: Select exactly one matching driver from borrowed table; NULL absent/ambiguous/invalid.
 * Driver-specific native parameters borrowed until its open returns; no fallback to another carrier.
 * Выбрать один driver из borrowed table; NULL absent/ambiguous/invalid, без fallback. */


/* [standalone-contract:physelect]
 * EN: physelect: remote_phy/select implementation and native state.
 * RU: physelect: реализация remote_phy/select и нативное состояние.
 * drivers (const PhyDriver *drivers): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * count (size_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * parameters (const void *parameters): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: RemotePhy *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: RemotePhy *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name, const void *parameters, size_t size);
#ifdef __cplusplus
}
#endif
#endif
