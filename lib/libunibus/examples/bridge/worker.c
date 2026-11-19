/* Copyright (c) 2026 bhgv
 * EN: Independent request FIFOs and per-client backend workers; no broker RX blocking on a client's request.
 * RU: Независимые FIFO запросов и backend workers клиентов; broker RX не блокируется на запросе клиента.
 */
#include "bridge.h" /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <stdlib.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */
#include <string.h> /* EN: Import declarations for this translation unit. RU: Подключить объявления этой единицы компиляции. */

/* EN/RU: See bridge.h; allocate before locking, transfer ownership only on successful enqueue.
 * См. bridge.h; выделить до блокировки, передать владение лишь при успешной постановке.
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
int bridge_enqueue(Worker *worker, const void *bytes, size_t size) /* EN: Invoke bridge_enqueue with these explicit arguments. RU: Вызвать bridge_enqueue с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    Packet *packet = malloc(sizeof(*packet) + size); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (!packet) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        return 0; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    packet->next = NULL; /* EN: Update packet->next for the current operation. RU: Обновить packet->next для текущей операции. */
    packet->size = size; /* EN: Update packet->size for the current operation. RU: Обновить packet->size для текущей операции. */
    memcpy(packet->bytes, bytes, size); /* EN: Copy the explicitly bounded byte span. RU: Скопировать явно ограниченный диапазон байтов. */
    pthread_mutex_lock(&worker->lock); /* EN: Acquire this state lock before modifying shared data. RU: Взять этот lock до изменения общих данных. */
    int accepted = !worker->ending && !worker->failed; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    if (accepted) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        if (worker->tail) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            worker->tail->next = packet; /* EN: Update worker->tail->next for the current operation. RU: Обновить worker->tail->next для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        else /* EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия. */
        { /* EN: Begin this scope. RU: Начало области. */
            worker->head = packet; /* EN: Update worker->head for the current operation. RU: Обновить worker->head для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        worker->tail = packet; /* EN: Update worker->tail for the current operation. RU: Обновить worker->tail для текущей операции. */
        pthread_cond_signal(&worker->ready); /* EN: Notify a waiting consumer of this queue. RU: Уведомить ожидающего потребителя этой очереди. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    pthread_mutex_unlock(&worker->lock); /* EN: Publish the protected updates and release this state lock. RU: Опубликовать защищённые изменения и снять lock. */
    if (!accepted) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
    { /* EN: Begin this scope. RU: Начало области. */
        free(packet); /* EN: Release the allocation after its final use. RU: Освободить allocation после последнего использования. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    return accepted; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */

/* EN/RU: See bridge.h; one outstanding 9P RPC per client is this example's explicit backend profile.
 * См. bridge.h; один незавершённый 9P RPC на клиента — явный backend-профиль этого примера.
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
void *bridge_worker(void *state) /* EN: Invoke bridge_worker with these explicit arguments. RU: Вызвать bridge_worker с этими явными аргументами. */
{ /* EN: Begin this scope. RU: Начало области. */
    Worker *worker = state; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    unsigned char reply[BRIDGE_LIMIT]; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
    for (;;) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
    { /* EN: Begin this scope. RU: Начало области. */
        pthread_mutex_lock(&worker->lock); /* EN: Acquire this state lock before modifying shared data. RU: Взять этот lock до изменения общих данных. */
        while (!worker->head && !worker->ending) /* EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла. */
        { /* EN: Begin this scope. RU: Начало области. */
            pthread_cond_wait(&worker->ready, &worker->lock); /* EN: Wait with this queue lock released, reacquiring it on wakeup. RU: Ждать со снятым lock очереди, повторно взяв его при пробуждении. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        Packet *packet = worker->head; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        if (packet) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            worker->head = packet->next; /* EN: Update worker->head for the current operation. RU: Обновить worker->head для текущей операции. */
            if (!worker->head) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
            { /* EN: Begin this scope. RU: Начало области. */
                worker->tail = NULL; /* EN: Update worker->tail for the current operation. RU: Обновить worker->tail для текущей операции. */
            } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        int failed = worker->failed; /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
        pthread_mutex_unlock(&worker->lock); /* EN: Publish the protected updates and release this state lock. RU: Опубликовать защищённые изменения и снять lock. */
        if (!packet) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            return NULL; /* EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        int success = !failed && bridge_exact(worker->socket, packet->bytes, packet->size, 1) && /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
                      bridge_exact(worker->socket, reply, 4, 0); /* EN: Invoke bridge_exact with these explicit arguments. RU: Вызвать bridge_exact с этими явными аргументами. */
        unsigned length = success ? bridge_length(reply) : 0; /* EN: Invoke bridge_length with these explicit arguments. RU: Вызвать bridge_length с этими явными аргументами. */
        success = /* EN: Update success for the current operation. RU: Обновить success для текущей операции. */
            success && length >= 7 && length <= sizeof(reply) && bridge_exact(worker->socket, reply + 4, length - 4, 0); /* EN: Invoke sizeof with these explicit arguments. RU: Вызвать sizeof с этими явными аргументами. */
        free(packet); /* EN: Release the allocation after its final use. RU: Освободить allocation после последнего использования. */
        if (success) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            success = bridge_reply(worker->tag, 1, reply, length); /* EN: Update success for the current operation. RU: Обновить success для текущей операции. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        else /* EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия. */
        { /* EN: Begin this scope. RU: Начало области. */
            bridge_reply(worker->tag, 2, "backend connection failed", 25); /* EN: Invoke bridge_reply with these explicit arguments. RU: Вызвать bridge_reply с этими явными аргументами. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
        if (!success) /* EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию. */
        { /* EN: Begin this scope. RU: Начало области. */
            pthread_mutex_lock(&worker->lock); /* EN: Acquire this state lock before modifying shared data. RU: Взять этот lock до изменения общих данных. */
            worker->failed = 1; /* EN: Update worker->failed for the current operation. RU: Обновить worker->failed для текущей операции. */
            pthread_mutex_unlock(&worker->lock); /* EN: Publish the protected updates and release this state lock. RU: Опубликовать защищённые изменения и снять lock. */
        } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
    } /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
} /* EN: Finish this scope or grouped value. RU: Конец области либо составного значения. */
