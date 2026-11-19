/* Copyright (c) 2026 bhgv
 * EN: Private two-client opaque-9P gateway state; protocol clients never see outer transport tag0.
 * RU: Частное состояние двухклиентского шлюза непрозрачного 9P; клиенты не видят внешний транспортный tag0.
 */
#ifndef EXAMPLE_BRIDGE_H /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#define EXAMPLE_BRIDGE_H /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
#include <pthread.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stddef.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#define BRIDGE_LIMIT 8192U /* Maximum standard 9P frame in this example profile. */

/* EN/RU: One copied client request, owned by its queue then its worker.
 * Один скопированный запрос клиента, принадлежащий очереди, затем worker.
 */
typedef struct Packet /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
{ /* EN: Begin this scope. RU: Начало области. */
    struct Packet *next;   /* Owned FIFO continuation. */
    size_t size;           /* Exact original 9P extent. */
    unsigned char bytes[]; /* Copied original protocol bytes. */
} Packet; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */

/* EN/RU: One independently serviced client with its own connected socket and FIFO.
 * Один независимо обслуживаемый клиент со своим подключённым socket и FIFO.
 */
typedef struct Worker /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
{ /* EN: Begin this scope. RU: Начало области. */
    pthread_mutex_t lock; /* Protects queue and terminal flags, never held during socket I/O. */
    pthread_cond_t ready; /* Wakes only this client's native consumer. */
    pthread_t thread;     /* Joined before socket/queue disposal. */
    Packet *head;         /* Owned oldest queued request. */
    Packet *tail;         /* Borrowed newest queued request. */
    int ending;           /* Input EOF requested a drain and shutdown. */
    int failed;           /* Sticky failure, prevents new effects on this connection. */
    int socket;           /* Exclusive connected backend socket. */
    unsigned long tag;    /* Immutable outer client assignment7 or8. */
} Worker; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */

/* EN/RU: Transfer exactly size bytes on an already owned descriptor;1/0, no replay after failure.
 * Передать ровно size байт по удержанному дескриптору;1/0, без повтора после ошибки.
 */


/* [standalone-contract:bridge_exact]
 * EN: bridge_exact: bridge/io implementation and native state.
 * RU: bridge_exact: реализация bridge/io и нативное состояние.
 * descriptor (int descriptor): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * bytes (void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * writing (int writing): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_exact(int descriptor, void *bytes, size_t size, int writing); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN/RU: Encode/write one complete outer record, serialized only with other output;1/0.
 * Кодировать/записать одну внешнюю запись, сериализуя только вывод;1/0.
 */


/* [standalone-contract:bridge_record]
 * EN: bridge_record: bridge/io implementation and native state.
 * RU: bridge_record: реализация bridge/io и нативное состояние.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_record(unsigned long tag, const void *bytes, size_t size); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN/RU: Wrap client bytes in inner tag1 (reply) or2 (error), then the immutable outer tag;1/0.
 * Обернуть байты клиента внутренним tag1 (ответ) либо2 (ошибка), затем внешним tag;1/0.
 */


/* [standalone-contract:bridge_reply]
 * EN: bridge_reply: bridge/io implementation and native state.
 * RU: bridge_reply: реализация bridge/io и нативное состояние.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * inner (unsigned long inner): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_reply(unsigned long tag, unsigned long inner, const void *bytes, size_t size); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN/RU: Validate copied 9P frame length; no interpretation of application fids or operations.
 * Проверить длину скопированного 9P frame; без трактовки fid и операций приложения.
 */


/* [standalone-contract:bridge_length]
 * EN: bridge_length: bridge/io implementation and native state.
 * RU: bridge_length: реализация bridge/io и нативное состояние.
 * bytes (const unsigned char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: unsigned; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
unsigned bridge_length(const unsigned char *bytes); /* EN: Invoke bridge_length with these explicit arguments. RU: Вызвать bridge_length с этими явными аргументами. */


/* EN/RU: Copy/enqueue one request on its client only;1/0, never waits for backend execution.
 * Скопировать/поставить запрос только своему клиенту;1/0, не ждать исполнения backend.
 */


/* [standalone-contract:bridge_enqueue]
 * EN: bridge_enqueue: bridge/worker implementation and native state.
 * RU: bridge_enqueue: реализация bridge/worker и нативное состояние.
 * worker (Worker *worker): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int bridge_enqueue(Worker *worker, const void *bytes, size_t size); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */


/* EN/RU: Own one client's serial backend I/O until input ends/fails; borrowed Worker stays live through join.
 * Обслуживать последовательный I/O клиента до EOF/ошибки; Worker заимствован до join.
 */


/* [standalone-contract:bridge_worker]
 * EN: bridge_worker: bridge/worker implementation and native state.
 * RU: bridge_worker: реализация bridge/worker и нативное состояние.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *bridge_worker(void *state); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
#endif /* EN: Select the declared compile-time interface/profile. RU: Выбрать объявленный compile-time интерфейс или профиль. */
