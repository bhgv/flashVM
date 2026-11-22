/* Copyright (c) 2026 bhgv
 * EN: Own a reverse Styx session exposing one Unibus inbox.
 * RU: Встречная сессия Styx с одним приёмником Unibus.
 */
#ifndef STYX_UNIBUS_SERVER_H
#define STYX_UNIBUS_SERVER_H
#include "unibus_inbox.h"
#include "phy.h"

#define STYX_UNIBUS_REPLY_TAG 17UL /* One-shot terminal launch completion tag. */

typedef struct StyxUnibusServer StyxUnibusServer; /* Owned reverse server and receive provider. */


/* EN/RU: Consume channel on every outcome; return owned server or NULL.
 * Принять канал при любом исходе; вернуть свой сервер или NULL. */
StyxUnibusServer *styxunibusserverstart(RemotePhy *channel, unsigned long tag);


/* EN/RU: Also export a borrowed exact original node as read-only implementation.dis.
 * Также опубликовать заимствованный точный узел implementation.dis на чтение. */
StyxUnibusServer *styxunibusserverstartfile(RemotePhy *channel, unsigned long tag, DVM_Provider *original);


/* EN/RU: Also export the optional borrowed profile as read-only .interface.
 * Также опубликовать заимствованный профиль как .interface на чтение. */
StyxUnibusServer *styxunibusserverstartmodule(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);


/* EN/RU: Like startmodule, with a reusable single-record inbox for serialized calls.
 * Как startmodule, с переиспользуемым приёмником для последовательных вызовов. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);


/* EN/RU: Consume channel and publish mounted/module/name, borrowing method state through join.
 * Принять канал и опубликовать mounted/module/name, заимствуя состояние метода до join. */
struct StyxScalarFunction;
StyxUnibusServer *styxunibusserverstartfunction(RemotePhy *channel, const char *module,
    const char *name, const struct StyxScalarFunction *function);


#define STYX_MOUNTED_SCALAR_LIMIT 64U /* Maximum export entries per server. */

typedef struct StyxMountedScalar
{
    const char *module; /* Borrowed directory name. */
    const char *name; /* Borrowed export name. */
    const struct StyxScalarFunction *native; /* Optional copied native method; NULL selects Dis. */
    int dis_big; /* Select the full-width signature for a Dis-backed entry. */
    int dis_tuple; /* Select the tuple ABI for a Dis-backed entry. */
    int dis_ref;   /* Select the scalar-field ref ABI for a Dis-backed entry. */
    int dis_call;  /* Select the argument-list ABI for a Dis-backed entry. */
    int dis_ref_pair; /* Select the two-ref ABI for a Dis-backed entry. */
} StyxMountedScalar;


/* EN/RU: Consume channel and publish count borrowed entries grouped by module name.
 * Принять канал и опубликовать count заимствованных записей с группировкой по имени модуля. */
StyxUnibusServer *styxunibusserverstartmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count);


/* EN/RU: Borrow indexed Dis request (reply=0) or reply mailbox until join.
 * Заимствовать приёмник запроса (reply=0) или ответа Dis по индексу до join. */
DVM_Provider *styxunibusservermailbox(StyxUnibusServer *server, size_t index, int reply);


/* EN/RU: Consume channel and expose a typed function serviced by the owner VM.
 * Принять канал и открыть типизированную функцию, обслуживаемую VM владельца. */
StyxUnibusServer *styxunibusserverstartdis(RemotePhy *channel, const char *module, const char *name);


/* EN/RU: Borrow the write mailbox of a Dis endpoint through server join.
 * Заимствовать приёмник записи конечной точки Dis до завершения сервера. */
DVM_Provider *styxunibusserverreply(StyxUnibusServer *server);


/* EN/RU: Borrow the local reader provider until server join.
 * Заимствовать локальный провайдер чтения до завершения сервера. */
DVM_Provider *styxunibusserverinbox(StyxUnibusServer *server);


/* EN/RU: After peer or carrier shutdown, join and consume server; NULL allowed.
 * После закрытия другой стороны или соединения дождаться и освободить сервер; NULL допустим. */
int styxunibusserverjoin(StyxUnibusServer *server);

/* EN: Consume channel and reserve one export. Its function and interface are
 * admitted only after the owner VM publishes the linked export to loadprofile.
 * RU: Принять канал и зарезервировать один экспорт. Функция и интерфейс
 * доступны после публикации связанного экспорта VM владельца в loadprofile. */
StyxUnibusServer *styxunibusserverstartloaded(RemotePhy *channel, const char *module, const char *name);


/* EN: Borrow the private writable profile mailbox through server join.
 * RU: Заимствовать служебный приёмник профиля на запись до join. */
DVM_Provider *styxunibusserverloadprofile(StyxUnibusServer *server);

/* EN: Consume channel and reserve an atomic directory of one to eight exports.
 * RU: Принять канал и зарезервировать каталог из одного–восьми экспортов. */
StyxUnibusServer *styxunibusserverstartloadedset(RemotePhy *channel, const char *module,
    const char *const *names, size_t count);


/* EN: Like loadedset; require a caller scope for shape-driven calls.
 * Borrow reverse, retaining it through join. Consume channel on all outcomes.
 * RU: Как loadedset, требовать контекст для вызовов по схеме.
 * Заимствовать reverse с удержанием до join. Всегда принять channel. */
StyxUnibusServer *styxunibusserverstartscopedset(RemotePhy *channel, const char *module,
    const char *const *names, size_t count, DVM_Provider *reverse);


/* EN: Publish loaded exports from multiple modules on one caller-aware session.
 * Borrow entries and reverse; retain reverse through join and consume channel on return.
 * RU: Опубликовать экспорты нескольких модулей в одной сессии с контекстом клиента.
 * Заимствовать entries и reverse, удерживать reverse до join, принять channel. */
StyxUnibusServer *styxunibusserverstartscopedtable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, DVM_Provider *reverse);


#define STYX_LAUNCH_EXTRA_LIMIT 16U /* Maximum additional runnable images. */
#define STYX_LAUNCH_NAME_LIMIT 31U /* Maximum length of an exported basename. */
#define STYX_LAUNCH_PROGRAMS "programs" /* Directory with named images and their request files. */
#define STYX_LAUNCH_CATALOG ".programs" /* Bounded list of published basenames. */

typedef struct StyxUnibusProgram
{
    const char *name; /* Public basename ending in .dis, copied when exported. */
    DVM_Provider *image; /* Borrowed image retained by the server through join. */
} StyxUnibusProgram;

/* EN: Check a public basename with no path components or hidden prefix.
 * RU: Проверить открытое имя .dis без элементов пути или скрытого префикса. */
int styxunibusprogramname(const char *name);

/* EN: Provider-owned launch publication on the same carrier as library exports.
 * RU: Публикация программы провайдера на одном транспорте с библиотеками. */
typedef struct StyxUnibusLaunch
{
    unsigned long tag; /* Nonzero S-record tag accepted by .unibus. */
    DVM_Provider *original; /* Borrowed original Dis image. */
    DVM_Provider *profile; /* Borrowed launch declaration. */
    int persistent; /* Queue scoped requests and attach a result to each fid. */
    const StyxUnibusProgram *programs; /* Additional persistent launch images, borrowed during setup. */
    size_t program_count; /* Number of additional images. */
    DVM_Provider *catalog; /* Borrowed immutable list of names, retained through join. */
} StyxUnibusLaunch;

/* EN: Publish a launch inbox, exit result and mounted native/Dis functions.
 * Consume channel; retain the launch files through join.
 * RU: Опубликовать запуск, результат и функции. Принять channel и удерживать
 * файлы запуска до join. */
StyxUnibusServer *styxunibusserverstartlaunchmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, const StyxUnibusLaunch *launch);

/* EN: Publish the launch endpoint with loaded, caller-scoped Dis exports.
 * Consume channel; retain reverse and both launch files through join.
 * RU: Опубликовать запуск и загруженные экспорты с контекстом клиента.
 * Принять channel, удерживать reverse и оба файла запуска до join. */
StyxUnibusServer *styxunibusserverstartscopedlaunchtable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, DVM_Provider *reverse, const StyxUnibusLaunch *launch);

/* EN: Publish persistent executable images without library exports. Consume channel.
 * RU: Опубликовать исполняемые образы без библиотечных экспортов. Принять channel. */
StyxUnibusServer *styxunibusserverstartprograms(RemotePhy *channel, const StyxUnibusLaunch *launch);

typedef struct StyxLaunchTicket StyxLaunchTicket;

/* EN: Return the borrowed ticket's image slot, zero for the default.
 * RU: Вернуть номер образа из билета, ноль для основного образа. */
size_t styxunibuslaunchslot(const StyxLaunchTicket *ticket);

/* EN: Wait on a persistent endpoint. wire holds at least 64 bytes. Return an
 * owned ticket and the exact request, or NULL on shutdown/cancel.
 * RU: Ждать на многоразовой точке запуска. wire вмещает не менее 64 байт.
 * Вернуть собственный билет и точный запрос либо NULL при остановке/отмене. */
StyxLaunchTicket *styxunibusservernextlaunch(StyxUnibusServer *server, void *wire, size_t capacity, size_t *size);

/* EN: Bind a borrowed ticket to the registered child once, before its first instruction.
 * The callback can run on the Styx dispatcher and must target this ticket identity.
 * RU: Привязать билет к зарегистрированному потомку перед первой инструкцией.
 * Callback может исполняться в диспетчере Styx и сверяет идентичность билета. */
void styxunibuslaunchstarted(StyxLaunchTicket *ticket, int pid, void (*cancel)(int, void *));

/* EN: Consume a ticket and publish its result. A nonzero status means failure.
 * RU: Поглотить билет и опубликовать результат. Ненулевой статус означает ошибку. */
void styxunibuslaunchcomplete(StyxLaunchTicket *ticket, int status);

/* EN: Publish one launch exit status while continuing to serve the session.
 * RU: Опубликовать результат запуска и продолжить обслуживание сессии. */
void styxunibusserverlaunchstatus(StyxUnibusServer *server, int status);


/* EN: Borrow the writable load offer for the indexed export until join.
 * RU: Заимствовать приёмник профиля экспорта по индексу до join. */
DVM_Provider *styxunibusserverloadprofileat(StyxUnibusServer *server, size_t index);

/* EN: Publish a one-shot module with an additional read-only exit file.
 * RU: Опубликовать одноразовый модуль с файлом результата только для чтения. */
StyxUnibusServer *styxunibusserverstartexit(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);

/* EN: Publish a scoped launch queue with no library exports. Consume the channel.
 * RU: Опубликовать очередь запуска с контекстом без библиотечных экспортов. Принять канал. */
StyxUnibusServer *styxunibusserverstartpersistent(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);

/* EN: Publish the exit status and keep the server available until EOF or timeout.
 * RU: Опубликовать статус и оставить сервер доступным до EOF или таймаута. */
void styxunibusserverexit(StyxUnibusServer *server, int status);

#endif
