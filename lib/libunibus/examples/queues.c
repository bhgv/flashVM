/* Copyright (c) 2026 bhgv
 * EN: A native consumer waits for only its client while the independent dispatcher serves another client.
 * RU: Нативный получатель ждёт только своего клиента, пока независимый диспетчер обслуживает другого.
 */
#include "order.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <pthread.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdio.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
static StyxOrder *receiver;     /* Owned main-thread engine, borrowed by joined consumer. */
static pthread_barrier_t ready; /* Test/demo gate establishes concurrent client lifetime without sleeps. */
static int received;            /* Written by the consumer and read by main only after join. */

/* EN/RU: Wait only on client7, then consume one exact record; return after publishing a scalar result.
 * Ждать только клиента7, затем прочитать одну запись; опубликовать скалярный результат и завершиться.
 */


/* [standalone-contract:consumer]
 * EN: consumer: examples/queues implementation and native state.
 * RU: consumer: реализация examples/queues и нативное состояние.
 * unused (void *unused): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void *consumer(void *unused) /* EN: Invoke consumer with these explicit arguments. RU: Вызвать consumer с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    unsigned char bytes[32]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t size; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned long long ordinal; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    (void)unused; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    pthread_barrier_wait(&ready); /* EN: Invoke pthread_barrier_wait with these explicit arguments. RU: Вызвать pthread_barrier_wait с этими явными аргументами. */
    received = styxorderwait(receiver, 7, 5000) && /* EN: Update received for the current operation. RU: Обновить received для текущей операции. */
               styxordernext(receiver, 7, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_ACCEPTED && /* EN: Invoke styxordernext with these explicit arguments. RU: Вызвать styxordernext с этими явными аргументами. */
               ordinal == 0 && size == 5 && !memcmp(bytes, "seven", 5); /* EN: Compare the exact byte span, including embedded zero bytes. RU: Сравнить точный диапазон, включая нулевые байты. */
    return NULL; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: Feed independent client8 while client7 waits; then deliver client7 and join, preserving exact uint64 order.
 * Передать клиенту8, пока клиент7 ждёт; затем передать клиенту7 и выполнить join с точным uint64-порядком.
 */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0, 1, 2. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1, 2. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(void) /* EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    StyxOrder *sender = styxordercreate(256, 16); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    receiver = styxordercreate(256, 16); /* EN: Update receiver for the current operation. RU: Обновить receiver для текущей операции. */
    unsigned char packet[512], bytes[32]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    size_t used, size; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned long long ordinal; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    pthread_t thread; /* EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами. */
    if (!sender || !receiver || pthread_barrier_init(&ready, NULL, 2) || pthread_create(&thread, NULL, consumer, NULL)) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 1; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    pthread_barrier_wait(&ready); /* EN: Invoke pthread_barrier_wait with these explicit arguments. RU: Вызвать pthread_barrier_wait с этими явными аргументами. */
    int good = styxorderencode(sender, 8, "eight", 5, packet, sizeof(packet), &used) == FVM_S_OK && /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
               styxorderfeed(receiver, packet, used) == STYX_ORDER_ACCEPTED && /* EN: Invoke styxorderfeed with these explicit arguments. RU: Вызвать styxorderfeed с этими явными аргументами. */
               styxordernext(receiver, 8, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_ACCEPTED && /* EN: Invoke styxordernext with these explicit arguments. RU: Вызвать styxordernext с этими явными аргументами. */
               ordinal == 0 && size == 5 && !memcmp(bytes, "eight", 5); /* EN: Compare the exact byte span, including embedded zero bytes. RU: Сравнить точный диапазон, включая нулевые байты. */
    good = good && styxorderencode(sender, 7, "seven", 5, packet, sizeof(packet), &used) == FVM_S_OK && /* EN: Update good for the current operation. RU: Обновить good для текущей операции. */
           styxorderfeed(receiver, packet, used) == STYX_ORDER_ACCEPTED; /* EN: Invoke styxorderfeed with these explicit arguments. RU: Вызвать styxorderfeed с этими явными аргументами. */
    int joined = pthread_join(thread, NULL); /* EN: Wait for the owned worker to finish before reclaiming its state. RU: Дождаться своего worker до освобождения его состояния. */
    pthread_barrier_destroy(&ready); /* EN: Invoke pthread_barrier_destroy with these explicit arguments. RU: Вызвать pthread_barrier_destroy с этими явными аргументами. */
    styxorderclose(receiver); /* EN: Invoke styxorderclose with these explicit arguments. RU: Вызвать styxorderclose с этими явными аргументами. */
    styxorderfree(receiver); /* EN: Invoke styxorderfree with these explicit arguments. RU: Вызвать styxorderfree с этими явными аргументами. */
    styxorderfree(sender); /* EN: Invoke styxorderfree with these explicit arguments. RU: Вызвать styxorderfree с этими явными аргументами. */
    if (!good || joined || !received) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 2; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    puts("PASS independent dispatcher/client waits, client8 progress and client7 exact ordered delivery"); /* EN: Invoke puts with these explicit arguments. RU: Вызвать puts с этими явными аргументами. */
    return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
