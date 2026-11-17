/* Copyright (c) 2026 bhgv
 * EN: Shared player background implementation; included in owner-thread dependency order.
 * RU: Shared player background; подключается в порядке owner-thread зависимостей.
 */


/* Explicit background session borrows only owner-thread native window/audio pointers. */
typedef struct FVM_DesktopBackgroundSession
{
    FVM_DesktopTransferOptions options;    /* Immutable provider policy copied before playback. */
    FVM_DesktopWindow *window;             /* Borrowed only while play_bytes owns a live view. */
    FVM_DesktopAudio *audio;               /* Borrowed only while its native output is live. */
    FVM_DesktopPlaybackSession *control;   /* Borrowed owner policy through source-run return. */
    int closed;                            /* Owner-thread close request, independent of worker status. */
    int asynchronous;                      /* Keep the timeline running for deferred network child/form jobs. */
    const FVM_DesktopLoadMonitor *monitor; /* Borrowed owner-only callbacks and deadline policy. */
    unsigned long next_id;                 /* One-based request IDs, bounded by validated load budget. */
    unsigned int kind;                     /* Source or synchronous child/form purpose for background_fetch. */
} FVM_DesktopBackgroundSession;


/* Report borrowed metadata only; terminal callbacks cannot revoke a published disposition. */
static void background_notice(FVM_DesktopBackgroundSession *session,
                              unsigned long id,
                              unsigned int kind,
                              unsigned int phase,
                              const char *reference,
                              const char *base,
                              const FVM_DesktopTransferView *view,
                              FVM_Result result)
{
    FVM_DesktopLoadEvent event;
    int stop;
    if (!session->monitor || !session->monitor->notify)
    {
        return;
    }
    event.id = id;
    event.kind = kind;
    event.phase = phase;
    event.received = view ? view->received : 0;
    event.result = result;
    event.reference = reference;
    event.base = base;
    event.location = phase == FVM_DESKTOP_LOAD_FETCHED && view ? view->location : 0;
    stop = session->monitor->notify(&event, session->monitor->userdata);
    if (stop && (phase == FVM_DESKTOP_LOAD_BEGIN || phase == FVM_DESKTOP_LOAD_WAIT))
    {
        session->closed = 1;
    }
}


/* Begin notification precedes I/O; an owner stop does not start a provider. */
static FVM_Result background_begin(FVM_DesktopBackgroundSession *session,
                                   const char *reference,
                                   const char *base,
                                   unsigned int kind,
                                   FVM_DesktopTransfer **out,
                                   unsigned long *id)
{
    FVM_Result result;
    *id = ++session->next_id;
    background_notice(session, *id, kind, FVM_DESKTOP_LOAD_BEGIN, reference, base, 0, FVM_NOT_FOUND);
    result = session->closed
                 ? FVM_ERROR
                 : fvm_desktop_transfer_begin_timed(
                       reference, base, &session->options, session->monitor ? session->monitor->timeout_ms : 0, out);
    if (result != FVM_OK)
    {
        background_notice(session,
                          *id,
                          kind,
                          session->closed ? FVM_DESKTOP_LOAD_CANCELLED : FVM_DESKTOP_LOAD_FAILED,
                          reference,
                          base,
                          0,
                          result);
    }
    return result;
}


/* Map native result before its borrowed location is released. No VM import is implied. */
static void background_terminal(FVM_DesktopBackgroundSession *session,
                                unsigned long id,
                                unsigned int kind,
                                const char *reference,
                                const char *base,
                                const FVM_DesktopTransferView *view,
                                FVM_Result result,
                                int abandoned)
{
    unsigned int phase = abandoned ? FVM_DESKTOP_LOAD_CANCELLED : FVM_DESKTOP_LOAD_FAILED;
    if (!abandoned && view)
    {
        if (view->state == FVM_DESKTOP_TRANSFER_TIMED_OUT)
        {
            phase = FVM_DESKTOP_LOAD_TIMED_OUT;
        }
        else if (view->state == FVM_DESKTOP_TRANSFER_CANCELLED)
        {
            phase = FVM_DESKTOP_LOAD_CANCELLED;
        }
        else if (result == FVM_OK)
        {
            phase = FVM_DESKTOP_LOAD_FETCHED;
        }
    }
    background_notice(session, id, kind, phase, reference, base, view, abandoned ? FVM_ERROR : result);
}


/* A cooperative deadline stops further VM turns even if a provider is slow to acknowledge it.
 * The actual job remains pending until its worker exits; only this owner's local view changes. */
static FVM_Result background_poll(FVM_DesktopTransfer *job, FVM_DesktopTransferView *view)
{
    FVM_Result result = fvm_desktop_transfer_poll(job, view);
    if (result == FVM_OK && view->state == FVM_DESKTOP_TRANSFER_PENDING)
    {
        unsigned int reason = fvm_desktop_transfer_stop_requested(job);
        if (reason)
        {
            view->state = reason;
            view->result = FVM_ERROR;
        }
    }
    return result;
}


/* The worker owns only bytes; event/audio pumping and the consumer callback stay on the owner. */
static FVM_Result background_fetch(const char *reference,
                                   const char *base,
                                   FVM_DesktopWrite write,
                                   void *write_userdata,
                                   char *resolved,
                                   size_t capacity,
                                   void *userdata)
{
    FVM_DesktopBackgroundSession *session = (FVM_DesktopBackgroundSession *)userdata;
    FVM_DesktopTransfer *job = 0;
    FVM_DesktopTransferView view;
    unsigned long id;
    FVM_Result result = background_begin(session, reference, base, session->kind, &job, &id);
    memset(&view, 0, sizeof(view));
    if (result != FVM_OK)
    {
        return result;
    }
    for (;;)
    {
        result = background_poll(job, &view);
        if (result != FVM_OK || view.state != FVM_DESKTOP_TRANSFER_PENDING)
        {
            break;
        }
        background_notice(session, id, session->kind, FVM_DESKTOP_LOAD_WAIT, reference, base, &view, FVM_NOT_FOUND);
        if (session->closed)
        {
            break;
        }
        if (session->window)
        {
            if (!fvm_desktop_window_events(session->window, 0))
            {
                session->closed = 1;
            }
            result = fvm_desktop_window_status(session->window);
            if (result == FVM_OK && session->audio && session->audio->device)
            {
                result =
                    fvm_desktop_window_active(session->window)
                        ? fvm_desktop_audio_tick_paused(session->audio, session->control && session->control->paused)
                        : fvm_desktop_audio_pause(session->audio, 1);
            }
            if (session->closed || result != FVM_OK)
            {
                break;
            }
        }
        SDL_Delay(FVM_DESKTOP_BACKGROUND_WAIT_MS);
    }
    if (session->closed)
    {
        result = FVM_ERROR;
    }
    else if (result == FVM_OK)
    {
        result = view.result;
        if (result == FVM_OK)
        {
            if (!resolved || !capacity || strlen(view.location) >= capacity)
            {
                result = FVM_INVALID_ARGUMENT;
            }
            else
            {
                result = write(view.bytes, view.size, write_userdata);
                if (result == FVM_OK)
                {
                    strcpy(resolved, view.location);
                }
            }
        }
    }
    background_terminal(session, id, session->kind, reference, base, &view, result, session->closed);
    fvm_desktop_transfer_release(job); /* Always cancel/join before session or VM teardown. */
    return result;
}
