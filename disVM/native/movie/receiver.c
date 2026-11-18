/* Copyright (c) 2026 bhgv
 * EN: Native synchronized offer receiver; only validated current-generation bytes reach the player.
 * RU: Синхронизированный native receiver; player получает только validated current-generation bytes.
 */
#include "session_internal.h"
#include "fetch_internal.h"
#include "mount_cancel.h"
#include "announcement.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* EN/RU: Trusted native session retains its exact client, synchronized generation and bounded control decoder.
 * Trusted native session удерживает exact client, synchronized generation и bounded control decoder.
 */
struct StyxMovieSession
{
    FVM_PortSync *lock; /* Owned short state lock; never held during network/VM work. */
    size_t references; /* Public owners plus admitted native terminal watcher. */
    size_t owners; /* Public callers, including an autonomous monitor. */
    int automatic; /* Exclusive native monitor feed, never concurrent with manual feed. */
    StyxClient *client; /* Owned connection pin; root/control association never changes. */
    StyxWatch *watch; /* Owned terminal subscription, detached by base owner. */
    unsigned long long generation; /* Highest validated scoped offer, initially0. */
    int ready; /* Current generation may walk a movie root. */
    int closed; /* Sticky control/connection failure or explicit retirement. */
    FVM_SBindings bindings; /* Control direction's bounded binding registry. */
    FVM_SBinding slots[FVM_SBINDINGS_MAX]; /* Owned native binding metadata. */
    unsigned char input[MOVIE_FRAME]; /* Retained incomplete control record. */
    size_t used; /* Prefix extent, bounded by sizeof(input). */
};


/* EN/RU: Consume one native receiver pin after all admitted callbacks/operations retire.
 * Поглотить native pin после завершения admitted callbacks/operations. */
static void release(void *state)
{
    StyxMovieSession *session = state;
    fvm_port_sync_lock(session->lock);
    int last = --session->references == 0;
    fvm_port_sync_unlock(session->lock);
    if (last)
    {
        styxclientfree(session->client);
        fvm_port_sync_destroy(session->lock);
        free(session);
    }
}


/* EN/RU: Sticky native terminal observation; no network, managed entry or target wait.
 * Sticky native terminal observation без network/managed entry/target wait. */
static void terminal(void *state, int reason)
{
    StyxMovieSession *session = state;
    (void)reason;
    fvm_port_sync_lock(session->lock);
    session->closed = 1;
    session->ready = 0;
    fvm_port_sync_unlock(session->lock);
}


/* EN/RU: Observe authoritative local link state before taking receiver lock; no RPC or notice wait.
 * Sticky native notifications may still be queued after shutdown has already completed.
 * Проверить local link state до receiver lock без RPC/ожидания notice; shutdown уже мог завершиться. */
static void observe_link(StyxMovieSession *session)
{
    if (!styxclientalive(session->client))
    {
        terminal(session, 0);
    }
}


/* EN/RU: See receiver.h; callback state owns a separate pin across unsubscribe races.
 * См. receiver.h; callback state имеет отдельный pin на время unsubscribe races. */
StyxMovieSession *styxmoviesession(StyxClient *client)
{
    if (!client)
    {
        return NULL;
    }
    StyxMovieSession *session = calloc(1, sizeof(*session));
    if (!session)
    {
        return NULL;
    }
    session->lock = fvm_port_sync_create();
    if (!session->lock || !styxclientretain(client))
    {
        if (session->lock)
        {
            fvm_port_sync_destroy(session->lock);
        }
        free(session);
        return NULL;
    }
    session->client = client;
    session->references = 2;
    session->owners = 1;
    fvm_sbindings_init(&session->bindings, session->slots, FVM_SBINDINGS_MAX);
    session->watch = styxclientwatch(client, terminal, session, release);
    if (!session->watch)
    {
        release(session);
        release(session);
        return NULL;
    }
    return session;
}


/* EN/RU: Validate one agreed offer payload under session lock, without transport/tag interpretation.
 * Проверить agreed offer payload под session lock, без transport/tag interpretation.
 */
static int offer(StyxMovieSession *session, const unsigned char *bytes, size_t size)
{
    unsigned state; /* Validated existing offer state. */
    unsigned long long generation; /* Validated canonical positive generation. */
    if (!movie_offer_parse(bytes, size, &state, &generation))
    {
        return 0;
    }
    if (generation < session->generation)
    {
        return 1; /* Delayed older traffic cannot replace newer authority. */
    }
    int ready = state == MOVIE_OFFER_READY;
    if (generation == session->generation)
    {
        return ready == session->ready;
    }
    session->generation = generation;
    session->ready = ready;
    session->closed = !ready;
    return 1;
}


/* EN/RU: Validate one complete record under state lock, before changing accepted generation.
 * Проверить complete record под state lock до изменения accepted generation. */
static int accept(StyxMovieSession *session, const FVM_SRecord *record, size_t consumed)
{
    if (session->closed)
    {
        return 0;
    }
    if (!record->tag)
    {
        size_t size;
        return fvm_sbindings_apply(&session->bindings, session->input, consumed, &size) == FVM_S_OK;
    }
    FVM_SBinding binding;
    if (fvm_sbindings_lookup(&session->bindings, record->tag, &binding) != FVM_S_OK)
    {
        return 0;
    }
    if (binding.type_size != sizeof(MOVIE_OFFER_TYPE)-1 ||
        memcmp(binding.type, MOVIE_OFFER_TYPE, sizeof(MOVIE_OFFER_TYPE)-1))
    {
        return 1;
    }
    return binding.version == MOVIE_OFFER_VERSION && offer(session, record->payload, record->size);
}


/* EN/RU: Borrow session/bytes/size;1 accepted/0 refusal, retain coalesced tails.
 * automatic1 requires monitor claim,0 requires manual ownership; corrupt input retires receiver.
 * Borrow session/bytes/size;1/0, сохранить tails; automatic1 для monitor,0 для manual owner. */
static int control(StyxMovieSession *session, const void *bytes, size_t size, int automatic)
{
    if (!session || (!bytes && size))
    {
        return 0;
    }
    fvm_port_sync_lock(session->lock);
    if (session->closed || session->automatic != automatic)
    {
        fvm_port_sync_unlock(session->lock);
        return 0;
    }
    const unsigned char *input = bytes;
    while (size)
    {
        size_t chunk = sizeof(session->input)-session->used;
        if (chunk > size)
        {
            chunk = size;
        }
        if (!chunk)
        {
            goto invalid;
        }
        memcpy(session->input+session->used, input, chunk);
        session->used += chunk;
        input += chunk;
        size -= chunk;
        for (;;)
        {
            FVM_SRecord record;
            size_t consumed;
            FVM_SResult status = fvm_srecord_read(session->input, session->used, MOVIE_FRAME, &record, &consumed);
            if (status == FVM_S_MORE)
            {
                break;
            }
            if (status != FVM_S_OK || !accept(session, &record, consumed))
            {
                goto invalid;
            }
            session->used -= consumed;
            memmove(session->input, session->input+consumed, session->used);
            if (!session->used)
            {
                break;
            }
        }
    }
    fvm_port_sync_unlock(session->lock);
    return 1;
invalid:
    session->closed = 1;
    session->ready = 0;
    fvm_port_sync_unlock(session->lock);
    return 0;
}


/* EN/RU: See receiver.h; manual feed refuses while an autonomous reader owns the direction.
 * См. receiver.h; manual feed запрещён при autonomous reader ownership. */
int styxmoviecontrol(StyxMovieSession *session, const void *bytes, size_t size)
{
    return control(session, bytes, size, 0);
}


/* EN/RU: See session_internal.h; only the monitor that holds the exclusive claim feeds here.
 * См. session_internal.h; feed только для monitor с exclusive claim. */
int movie_session_feed(StyxMovieSession *session, const void *bytes, size_t size)
{
    return control(session, bytes, size, 1);
}


/* EN/RU: See session_internal.h; consume the supplied fid even on refusal, native I/O outside state mutex.
 * См. session_internal.h; поглотить переданный fid даже при отказе, native I/O вне state mutex. */
FVM_Result movie_session_fetch_fid(StyxMovieSession *session, StyxFid *fid, unsigned long long expected,
    unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity)
{
    if (!session)
    {
        styxclientclunklater(fid);
        return FVM_INVALID_ARGUMENT;
    }
    if (!reference || !*reference || strlen(reference) >= MOVIE_PATH ||
        (base && strlen(base) >= MOVIE_PATH) || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA))
    {
        styxmountclunk(session->client, fid);
        return FVM_INVALID_ARGUMENT;
    }
    observe_link(session);
    fvm_port_sync_lock(session->lock);
    unsigned long long generation = session->generation;
    int ready = session->ready && !session->closed;
    fvm_port_sync_unlock(session->lock);
    if (!ready || (expected && generation != expected) || !fid)
    {
        styxmountclunk(session->client, fid);
        return FVM_ERROR;
    }
    if (styxclientfiddirectory(fid) != 0)
    {
        styxmountclunk(session->client, fid);
        return FVM_ERROR;
    }
    char origin[MOVIE_PATH]; /* Private through generation/link fence, even for the raw session API. */
    FVM_Result result = movie_fetch_fid(session->client, fid, purpose, reference, base, write, state,
        origin, capacity < sizeof(origin) ? capacity : sizeof(origin));
    observe_link(session);
    fvm_port_sync_lock(session->lock);
    if (session->closed || !session->ready || session->generation != generation || dvm_cancel_requested(dvm_cancel_current()))
    {
        result = FVM_ERROR;
    }
    else if (result == FVM_OK)
    {
        memcpy(resolved, origin, strlen(origin) + 1);
    }
    fvm_port_sync_unlock(session->lock);
    return result;
}


/* EN/RU: Legacy attached-root selection, no alternate namespace; the shared fid helper makes final admission.
 * Session/arguments borrowed, every acquired fid consumed. / Legacy выбор от attach root,
 * shared fid helper делает final admission; session/arguments borrowed, fid поглощается всегда.
 */
FVM_Result movie_session_fetch(StyxMovieSession *session, unsigned long long expected,
    unsigned purpose, const char *reference, const char *base, FVM_DesktopWrite write, void *state,
    char *resolved, size_t capacity)
{
    if (!session || !reference || !*reference || strlen(reference) >= MOVIE_PATH ||
        (base && strlen(base) >= MOVIE_PATH) || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA))
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned long long generation = 0; /* Snapshot before scoped walk; fid helper rechecks before open. */
    if (styxmovieavailable(session, &generation) != 1 || (expected && generation != expected))
    {
        return FVM_ERROR;
    }
    char name[64]; /* Existing canonical advertised service path. */
    snprintf(name, sizeof(name), "movie/%llu", generation);
    return movie_session_fetch_fid(session, styxmountwalkcancel(session->client, NULL, name), generation,
        purpose, reference, base, write, state, resolved, capacity);
}


/* EN/RU: See receiver.h; retain legacy selection of the current announced generation.
 * См. receiver.h; сохранить прежний выбор текущего объявленного generation.
 */
FVM_Result styxmovieannouncedfetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata)
{
    return movie_session_fetch(userdata, 0, purpose, reference, base, write, state, resolved, capacity);
}


/* EN/RU: See receiver.h; native coherent snapshot never enters the transport.
 * См. receiver.h; coherent snapshot без transport entry. */
int styxmovieavailable(StyxMovieSession *session, unsigned long long *generation)
{
    if (!session)
    {
        return -1;
    }
    observe_link(session);
    fvm_port_sync_lock(session->lock);
    int result = session->closed ? -1 : session->ready;
    if (generation)
    {
        *generation = session->generation;
    }
    fvm_port_sync_unlock(session->lock);
    return result;
}


/* EN/RU: See receiver.h; a partial pending record can never survive an ended control stream.
 * См. receiver.h; partial record не сохраняет authority после окончания control stream. */
void styxmoviecontrolend(StyxMovieSession *session)
{
    if (session)
    {
        terminal(session, 0);
    }
}


/* EN/RU: See receiver.h; retain only while caller already owns a live native reference.
 * См. receiver.h; retain только под существующим live native owner. */
int styxmoviesessionretain(StyxMovieSession *session)
{
    if (!session)
    {
        return 0;
    }
    fvm_port_sync_lock(session->lock);
    int valid = session->owners && session->owners != (size_t)-1 && session->references != (size_t)-1;
    if (valid)
    {
        session->owners++;
        session->references++;
    }
    fvm_port_sync_unlock(session->lock);
    return valid;
}


/* EN/RU: See session_internal.h; claim/unclaim setup under the same lock as feed.
 * См. session_internal.h; claim/unclaim setup под общим с feed lock. */
int movie_session_claim(StyxMovieSession *session, int acquire)
{
    fvm_port_sync_lock(session->lock);
    int valid = acquire ? !session->closed && !session->automatic && !session->used : session->automatic;
    if (valid)
    {
        session->automatic = acquire != 0;
    }
    fvm_port_sync_unlock(session->lock);
    return valid;
}


/* EN/RU: See session_internal.h; caller holds a session owner, client association is immutable.
 * См. session_internal.h; caller владеет session, client association immutable. */
StyxClient *movie_session_client(StyxMovieSession *session)
{
    return session->client;
}


/* EN/RU: See receiver.h; last public owner detaches native watcher once, outside receiver lock.
 * См. receiver.h; последний public owner завершает watcher один раз вне receiver lock. */
void styxmoviesessionfree(StyxMovieSession *session)
{
    if (session)
    {
        fvm_port_sync_lock(session->lock);
        int last = --session->owners == 0;
        fvm_port_sync_unlock(session->lock);
        if (last)
        {
            styxclientunwatch(session->watch);
        }
        release(session);
    }
}


/* EN/RU: See session_internal.h; automatic feed claim and no partial raw frame are mandatory.
 * См. session_internal.h; нужны automatic claim и отсутствие partial raw frame.
 */
int movie_session_offer(StyxMovieSession *session, const void *bytes, size_t size)
{
    if (!session || (!bytes && size))
    {
        return 0;
    }
    fvm_port_sync_lock(session->lock);
    int admitted = !session->closed && session->automatic && !session->used;
    int valid = admitted && offer(session, bytes, size);
    if (admitted && !valid)
    {
        session->closed = 1;
        session->ready = 0;
    }
    fvm_port_sync_unlock(session->lock);
    return valid;
}
