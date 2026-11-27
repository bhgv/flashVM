/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */


/* [standalone-contract:styxattachreply]
 * EN: styxattachreply: libstyx/attach implementation and native state.
 * RU: styxattachreply: реализация libstyx/attach и нативное состояние.
 * request (StyxAttach *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * error (char *error): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxattachreply(StyxAttach *, char *error); /* 1 consumed/attempted,0 retired/staged; nil Rattach. */


/* [standalone-contract:styxattachfree]
 * EN: styxattachfree: libstyx/attach implementation and native state.
 * RU: styxattachfree: реализация libstyx/attach и нативное состояние.
 * request (StyxAttach *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxattachfree(StyxAttach *); /* Drop caller ref; reply error before abandoning live work. */

/* POSIX borrowed readiness fd: owner registers fd or -1 to remove;0/-1.
 * Caller drains and closes it after unregister/end. Other ports return -1.
 * POSIX borrowed fd готовности; owner регистрирует, сам читает и закрывает. */


/* [standalone-contract:styxnotifyfd]
 * EN: styxnotifyfd: phy/backend implementation and native state.
 * RU: styxnotifyfd: реализация phy/backend и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnotifyfd(Styxserver *, int fd);

/* Native owner-thread byte-read extension, no new wire messages. Defer only during
 * read callback, once per non-directory/nonzero read and at most one per fid.
 * Set *n=0 and return nil on success. Returned request is caller-owned; NULL
 * leaves aux caller-owned. At most128 pending reads per client. No buffers retained.
 * Worker threads pass copied data through native mailboxes/fds, never request pointers.
 * Native byte-read extension; только read callback, *n=0/nil при успехе, свой request.
 * Worker передаёт copied data через mailbox/fd; request доступен только owner-thread. */


/* [standalone-contract:styxdeferread]
 * EN: styxdeferread: libstyx/read implementation and native state.
 * RU: styxdeferread: реализация libstyx/read и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * aux (void *aux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxRead *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: StyxRead *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxRead *styxdeferread(Styxserver *, void *aux, void (*release)(void *));

/* Borrow pending client/fid aux and exact capacity/offset only on native socket owner.
 * Client/aux become nil on retirement; zero limit then distinguishes inactivity.
 * Заимствовать client/fid aux, capacity/offset на owner; после retirement nil/zero. */


/* [standalone-contract:styxreadclient]
 * EN: styxreadclient: libstyx/read implementation and native state.
 * RU: styxreadclient: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Client *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Client *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Client *styxreadclient(StyxRead *);


/* [standalone-contract:styxreadaux]
 * EN: styxreadaux: libstyx/read implementation and native state.
 * RU: styxreadaux: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: void *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *styxreadaux(StyxRead *);


/* [standalone-contract:styxreadlimit]
 * EN: styxreadlimit: libstyx/read implementation and native state.
 * RU: styxreadlimit: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong styxreadlimit(StyxRead *);


/* [standalone-contract:styxreadoffset]
 * EN: styxreadoffset: libstyx/read implementation and native state.
 * RU: styxreadoffset: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: vlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: vlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
vlong styxreadoffset(StyxRead *);

/* Complete once; borrowed bytes copied during send, nil error means Rread.
 * Invalid count/buffer leaves request pending and returns0; inactive/staged returns0.
 * Return1 consumes live transaction/attempts response, not peer acknowledgement.
 * release(aux) runs once after detachment, before fid/client cleanup; never wait or
 * reenter dispatch/teardown. It may free the caller request ref. Flush/reset/disconnect
 * cancel without late replies; fid retirement sends Rerror before its clunk reply.
 * Завершить один раз; bytes borrowed до возврата.1 attempt,0 inactive/invalid/staged.
 * release(aux) один раз после detach до fid/client cleanup; без ожидания/reentry. */


/* [standalone-contract:styxreadreply]
 * EN: styxreadreply: libstyx/read implementation and native state.
 * RU: styxreadreply: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * count (ulong count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * error (char *error): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxreadreply(StyxRead *, void *bytes, ulong count, char *error);

/* Consume caller ref on owner; freeing a live handle alone does not answer/cancel it.
 * Поглотить caller ref на owner; само освобождение не отвечает/не отменяет запрос. */


/* [standalone-contract:styxreadfree]
 * EN: styxreadfree: libstyx/read implementation and native state.
 * RU: styxreadfree: реализация libstyx/read и нативное состояние.
 * request (StyxRead *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxreadfree(StyxRead *);
/* EN: Stage one deferred Twrite from the current write callback. On success
 * set its count to zero and return nil. Copy input before returning. The
 * request owns no payload. The shared byte I/O quota is 128 per client and
 * one per fid. Release aux once on retirement, on the server owner. On failure
 * return nil and leave aux with the caller.
 * RU: Отложить текущий Twrite из колбэка записи. При успехе обнулить count и
 * вернуть nil. Скопировать вход до возврата. Запрос не владеет данными.
 * Общий предел I/O равен 128 на клиента и одному на fid. Освободить aux
 * один раз у владельца при снятии. При отказе вернуть nil и сохранить aux. */
StyxWrite *styxdeferwrite(Styxserver *, void *aux, void (*release)(void *));


/* EN: Complete a waiting Twrite once on the dispatcher. With nil error,
 * count is the acknowledged prefix. A non-nil error requires count zero.
 * Return 1 when retired, 0 on invalid, staged or already retired requests.
 * RU: Однократно завершить ожидающий Twrite у диспетчера. При nil error
 * count задаёт подтверждённый префикс. При ошибке count равен нулю.
 * Вернуть 1 после снятия, 0 для неверного, ещё не принятого или снятого запроса. */
int styxwritereply(StyxWrite *, ulong count, char *error);


/* EN: Consume the caller's owned reference on the dispatcher. NULL is harmless.
 * RU: Освободить ссылку вызывающего у диспетчера. NULL допустим. */
void styxwritefree(StyxWrite *);
