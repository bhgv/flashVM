/* Copyright (c) 2026 bhgv
 * EN: Native-only retirement of abandoned walk fids without delaying cancelled callers.
 * RU: Native-only retirement abandoned walk fids без задержки cancelled callers.
 */
#include "internal.h"

/* EN/RU: Independent cleanup owns a client pin, never provider/Prog/guest state.
 * Independent cleanup владеет client pin, без provider/Prog/guest state. */
typedef struct Forgotten
{
    StyxClient *client; /* Owned independent native connection reference. */
    u32int number;      /* Monotonic abandoned fid number, never reused. */
} Forgotten;

/* EN/RU: Consume job, attempt one bounded clunk, then release its native client pin.
 * Поглотить job, выполнить один bounded clunk, освободить native client pin. */


/* [standalone-contract:retire]
 * EN: retire: native client forget operations and lifetime.
 * RU: retire: нативные client-операции forget и время жизни.
 * opaque (void *opaque): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void *retire(void *opaque)
{
    Forgotten *job = opaque;
    pthread_mutex_lock(&job->client->lock);
    styxclunknumber(job->client, job->number);
    pthread_mutex_unlock(&job->client->lock);
    styxclientfree(job->client);
    free(job);
    return NULL;
}

/* EN/RU: See internal.h; all fallible setup precedes publishing worker ownership.
 * См. internal.h; fallible setup выполняется до публикации worker ownership. */


/* [standalone-contract:styxforgetlocked]
 * EN: styxforgetlocked: native client forget operations and lifetime.
 * RU: styxforgetlocked: нативные client-операции forget и время жизни.
 * client (StyxClient *client): Native client/session context; retain or consume it exactly as specified by this API.
 * RU: Нативный контекст клиента/сессии; удерживать либо поглощать по контракту API.
 * number (u32int number): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxforgetlocked(StyxClient *client, u32int number)
{
    Forgotten *job = calloc(1, sizeof(*job));
    pthread_attr_t attributes;
    pthread_t worker;
    int initialized = job && !pthread_attr_init(&attributes);
    int valid = initialized && !pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) &&
                client->references != (size_t)-1;
    if (valid)
    {
        job->client = client;
        job->number = number;
        client->references++;
        valid = !pthread_create(&worker, &attributes, retire, job);
        if (!valid)
        {
            client->references--;
        }
    }
    if (initialized)
    {
        pthread_attr_destroy(&attributes);
    }
    if (!valid)
    {
        free(job);
        styxrpcbad(client);
    }
}
