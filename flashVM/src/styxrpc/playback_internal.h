/* Copyright (c) 2026 bhgv
 * EN: Private paired-playback state and owner dependencies.
 * RU: Внутреннее состояние парного плеера и его зависимости.
 */
#ifndef FLASHVM_STYXRPC_PLAYBACK_INTERNAL_H
#define FLASHVM_STYXRPC_PLAYBACK_INTERNAL_H
#include "flashvm/styxrpc_playback.h"
#include "movie_load.h"
#include "player_services.h"
#include "../../ports/styxrpc_stats_port.h"

typedef enum PlaybackState
{
    PLAYBACK_EMPTY,   /* No upload has passed validation. */
    PLAYBACK_READY,   /* Prepared movie at its initial timeline position. */
    PLAYBACK_RUNNING, /* Subsequent owner ticks advance the movie. */
    PLAYBACK_PAUSED,  /* Owner ticks leave the timeline unchanged. */
    PLAYBACK_DEBUG,   /* Paused with the interpreter and optional debugger configured. */
    PLAYBACK_FAILED   /* A timeline operation failed; restart can rebuild the image. */
} PlaybackState;

struct FVM_StyxRpcPlayback
{
    FVM_StyxRpcPlaybackSpec spec; /* Copied settings and borrowed host services. */
    struct StyxClient *named;    /* Borrowed channel service for a FlashVM pair. */
    const FVM_PeerRefs *refs;     /* Borrowed negotiated module-ref routing for this pair. */
    int awaiting_session;        /* Reject movie staging until the peer mount is attached. */
    int defer_command_tick;      /* Clocked player acknowledges start before executing the first frame. */
    StyxRpcControl *control;     /* Owns .upload and .state node callbacks. */
    FVM_Context *context;         /* Current movie context, NULL until a successful upload. */
    StyxRpcModuleSlot *slot;      /* Owned published tree while serving. */
    FVM_StyxRpcPlayerServices *script;  /* Owned per-movie AS session and launch tickets. */
    FVM_StyxRpcMovies *movies;   /* Owned deferred SWF loader for the current paired context. */
    FVM_Object *root;             /* [GC:] Root clip pinned in the current context. */
    unsigned char *swf;           /* Owned copy used by restart. */
    size_t size;                  /* Length of the retained SWF, in bytes. */
    PlaybackState state;         /* State reported to the controller. */
    FVM_Result last;              /* Last timeline or control operation status. */
    double period;                /* Milliseconds per frame from the uploaded SWF. */
    unsigned long schedule;        /* Changes when start establishes a new frame deadline. */
    unsigned long ticks;          /* Successfully advanced ticks for the current movie. */
    double last_wall;             /* Last available monotonic sample in seconds. */
    double last_cpu;              /* Last available owner CPU sample in seconds. */
    FVM_StyxRpcProcessStats process; /* Previous kernel sample for matching CPU deltas by thread identity. */
};


/* EN: Consume the local startup image on the owner. Run at the first clock tick on success.
 * RU: Принять стартовый образ у владельца. При успехе запустить на первом такте. */
int fvm_styxrpc_playback_start_local(FVM_StyxRpcPlayback *playback, unsigned char *bytes, size_t size);
#endif
