/* Confirmed host steps from a separate C89 controller; see README.md / README.ru.md for every API. */
#define SDL_MAIN_HANDLED /* Keep this tutorial's ordinary main rather than SDL's platform wrapper. */
#include "../../../frontend/common/player.h" /* Public mailbox/control API has no SDL or VM pointer in its snapshots. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* Use the explicitly selected build-time native provider. */
#include "../../../frontend/common/socket_http.h" /* Worker-safe native HTTP/TLS byte transport. */
#elif defined(FVM_DESKTOP_HTTP) /* A separate configuration may select worker-safe libcurl. */
#include "../../../frontend/common/http.h" /* Alternate provider with its own thread-safety checks. */
#endif /* Local/embedded sources do not require either provider. */
#include <SDL.h> /* This example uses SDL only as its replaceable OS-thread/atomic/clock adapter. */
#include <stdio.h> /* Report diagnostics and copied scalars after the worker joins. */
#include <string.h> /* Parse the console adapter's limited vocabulary. */
#include "movie.inc" /* Immutable linked SWF bytes, never treated as a C string. */
#define EXAMPLE_MAILBOX_MAX_STEPS 1000UL /* Bound the tutorial command count and numeric parser. */
#define EXAMPLE_MAILBOX_DEFAULT_STEPS 5UL /* Default number of individually confirmed host advances. */
#define EXAMPLE_MAILBOX_IDLE_MS 5000U /* Fail a stalled controller without a permanently paused runner. */
#define EXAMPLE_MAILBOX_POLL_MS 1U /* Controller polling delay; this worker never steals SDL UI events. */
#define EXAMPLE_MAILBOX_LOADS 8UL /* Startup and deferred child/form requests share this budget. */
#define EXAMPLE_MAILBOX_LOAD_TIMEOUT_MS 4000UL /* Cooperative per-network-job total deadline. */

/* Native host state remains alive until playback returns and the controller has joined. */
typedef struct Controller /* The worker holds no context/root/managed-value pointer. */
{ /* Only native synchronization and copied scalar progress cross threads. */
    FVM_DesktopPlaybackMailbox *mailbox; /* Borrow opaque native storage until after join. */
    unsigned long steps; /* Immutable requested count, set before starting this OS worker. */
    Uint32 started; /* Initialize the SDL clock on main before any controller can read it. */
    SDL_atomic_t done; /* Owner sets on every runner exit, including errors before READY. */
    SDL_atomic_t failed; /* Worker reports idle/protocol errors without entering VM. */
} Controller; /* An IDE can replace SDL with its own native-thread completion mechanism. */

/* Parse a positive bounded decimal completely, before any native resource is acquired. */
static int parse_steps(const char *text, unsigned long *out) /* This parser belongs only to the console adapter. */
{ /* Reject signs, whitespace, partial input and overflow without mutating output. */
    unsigned long value = 0; /* Zero is invalid rather than an unbounded-run selector. */
    if (!text || !*text) return 0; /* Require at least one digit. */
    while (*text) /* Consume the complete argument. */
    { /* Check the bound before multiplying, including on 32-bit processors. */
        unsigned long digit; /* Keep declarations before statements for C89. */
        if (*text < '0' || *text > '9') return 0; /* Accept ASCII decimal digits only. */
        digit = (unsigned long)(*text++ - '0'); /* Convert a validated digit. */
        if (value > (EXAMPLE_MAILBOX_MAX_STEPS - digit) / 10UL) return 0; /* No overflowing intermediate. */
        value = value * 10UL + digit; /* This value is within the declared step bound. */
    } /* All input bytes were accepted. */
    if (!value) return 0; /* A step demonstration requires a positive count. */
    *out = value; /* Publish only the validated complete number. */
    return 1; /* No VM, SDL subsystem or worker was started. */
} /* Library users configure descriptors directly without this parser. */

/* Native controller posts one step only after the preceding completed advance has been observed. */
static int controller_thread(void *raw) /* SDL provides the thread, not access to VM or UI events. */
{ /* Snapshots below contain owned-by-value text/scalars, independent of the VM heap. */
    Controller *host = (Controller *)raw; /* Borrow caller-owned state until joined. */
    FVM_DesktopPlaybackSnapshot snapshot; /* Caller storage for a consistent native copy. */
    FVM_Result result; /* Distinguish no observation yet from a native failure. */
    unsigned long issued = 0; /* Number of individual commands accepted by the mailbox. */
    Uint32 progressed = host->started; /* Main initialized the clock before launching this worker. */
    unsigned long completed = 0; /* Last acknowledged advance count, not a managed pointer. */
    fvm_desktop_playback_snapshot_init(&snapshot); /* Exact-size native output descriptor. */
    while (!SDL_AtomicGet(&host->done)) /* Early source failures must release this wait too. */
    { /* No VM, window, render, event-drain or GC function is called from this worker. */
        result = fvm_desktop_playback_mailbox_poll(host->mailbox, &snapshot); /* Brief native mutex only. */
        if (result != FVM_OK && result != FVM_NOT_FOUND) break; /* Stop through the common failure path. */
        if (result == FVM_OK) /* A root has reached a safe owner observation. */
        { /* Use completed counters, never assume post itself executed a frame. */
            if (snapshot.total_ticks != completed) /* Observe progress before granting another permit. */
            { /* A completed host tick refreshes the controller's idle budget. */
                completed = snapshot.total_ticks; /* Copy only a scalar observation. */
                progressed = SDL_GetTicks(); /* Unrelated pauses do not grant unbounded idle time. */
            } /* The next request is gated by issued-versus-completed counts. */
            if (completed >= host->steps) /* Every requested advance has been confirmed. */
                return fvm_desktop_playback_mailbox_post(host->mailbox, FVM_DESKTOP_PLAYBACK_STOP) == FVM_OK ? 0 : 1; /* Terminal intent. */
            if (snapshot.stop_requested) return 0; /* Another host component already ended this run. */
            if (completed == issued) /* At most one unacknowledged step is ever outstanding. */
            { /* Repeated posts before a callback would coalesce; this handshake avoids that. */
                if (fvm_desktop_playback_mailbox_post(host->mailbox, FVM_DESKTOP_PLAYBACK_STEP) != FVM_OK) break; /* No VM entry. */
                ++issued; /* Await the completed tick before posting the next permit. */
            } /* The actual owner applies the pending command at its next safe boundary. */
        } /* A NOT_FOUND snapshot during initial loading is ordinary, not an error. */
        if ((Uint32)(SDL_GetTicks() - progressed) >= EXAMPLE_MAILBOX_IDLE_MS) break; /* Wrap-safe short timeout. */
        SDL_Delay(EXAMPLE_MAILBOX_POLL_MS); /* Yield native CPU without blocking the owner/UI thread. */
    } /* Main signals done on every return, even when no first snapshot existed. */
    if (SDL_AtomicGet(&host->done)) return 0; /* Owner already cleaned up its run normally or with an error. */
    SDL_AtomicSet(&host->failed, 1); /* Report a controller-side idle/protocol failure to main after join. */
    fvm_desktop_playback_mailbox_post(host->mailbox, FVM_DESKTOP_PLAYBACK_STOP); /* Also cancel startup/blocking provider waits. */
    return 1; /* Native thread status is collected explicitly; no detached worker remains. */
} /* The worker never assumes its last copied source still identifies a live VM. */

/* Main/UI owner adapts arguments to descriptors and retains all native/input storage through cleanup. */
int main(int argc, char **argv) /* A native IDE can replace this adapter without constructing player arguments. */
{ /* Declare C89 storage before starting resource acquisition. */
    Controller host; /* Host-owned mailbox pointer, immutable command count and native completion flags. */
    FVM_DesktopOptions options; /* Playback scalars initialized by their own API. */
    FVM_DesktopSource source; /* Borrow a file/URL or the linked immutable bytes. */
    FVM_DesktopPlaybackControl control; /* Owner callback returns intent from the mailbox. */
    FVM_DesktopPlaybackSnapshot last; /* Independent final copy remains usable after mailbox release. */
    SDL_Thread *thread; /* Owned native join handle, never detached. */
    const char *error = 0; /* Borrowed runner diagnostic, never freed by this program. */
    int embedded = 0, selected_steps = 0; /* Reject duplicate console options before startup. */
    int valid = argc >= 2, argument, status, worker_status = 0; /* Keep argument/runner/thread outcomes separate. */
    int have_snapshot = 0; /* Early source failures may return before any READY observation. */
    memset(&host, 0, sizeof(host)); /* Initialize native atomic storage and owned pointers. */
    host.steps = EXAMPLE_MAILBOX_DEFAULT_STEPS; /* Default confirmed-step demonstration. */
    for (argument = 1; argument < argc - 1; ++argument) /* Treat the final argument as source provenance. */
    { /* The library itself remains independent of console syntax. */
        if (!strcmp(argv[argument], "--embedded") && !embedded) embedded = 1; /* Choose linked bytes once. */
        else if (!strcmp(argv[argument], "--steps") && !selected_steps && argument + 1 < argc - 1) /* Require a value. */
        { /* Consume and validate one bounded decimal count. */
            selected_steps = 1; /* A repeated selector is an argument error. */
            valid = parse_steps(argv[++argument], &host.steps) && valid; /* Preserve previous invalid input. */
        } /* All other tokens must match the explicit small vocabulary. */
        else valid = 0; /* Fail before VM/window/network/worker startup. */
    } /* argv storage stays borrowed throughout the later synchronous run. */
    if (!valid || argv[argc - 1][0] == '-') /* Do not reinterpret an unknown final option as a filename. */
    { /* Print the console adapter's complete syntax. */
        fprintf(stderr, "Usage: %s [--embedded] [--steps 1..1000] file-or-http[s]-URL\n", argv[0]); /* Required source/origin. */
        return 2; /* Argument error with no acquired native resources. */
    } /* Typed library descriptors handle further source/capability validation. */
    fvm_desktop_options_init(&options); /* Initialize all playback defaults. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* The tutorial does not require a native JIT backend. */
    options.mute = 1; options.stats = 1; /* Open no audio device and print actual execution counters. */
    options.frames = EXAMPLE_MAILBOX_MAX_STEPS + 1UL; /* STOP normally wins before this fallback frame bound. */
    fvm_desktop_playback_control_init(&control); /* Initialize the control descriptor and exact ABI size. */
    control.initially_paused = 1; /* No first tick occurs before the worker grants its first permit. */
    control.notify = fvm_desktop_playback_mailbox_notify; /* Owner-only copying adapter, with no VM pointers retained. */
    source.location = argv[argc - 1]; /* Explicit file/URL provenance also serves linked-buffer relative loading. */
    source.kind = strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE; /* Select startup I/O. */
    if (embedded) source.kind = FVM_DESKTOP_SOURCE_MEMORY; /* Skip startup I/O while retaining this explicit origin. */
    source.bytes = embedded ? embedded_movie : 0; /* Immutable storage outlives every callback/worker. */
    source.size = embedded ? sizeof(embedded_movie) : 0; /* Binary SWF bytes have an exact size, not strlen. */
#if !defined(FVM_DESKTOP_SOCKET_HTTP) && !defined(FVM_DESKTOP_HTTP) /* Keep local/memory use available in lean builds. */
    if (source.kind == FVM_DESKTOP_SOURCE_URL) /* This console adapter has no configured URL provider. */
    { /* Fail before allocating a mailbox or launching a controller. */
        fprintf(stderr, "URL loading requires a configured HTTP provider\n"); /* Explicit SDK capability diagnostic. */
        return 1; /* A custom C host may instead provide its own supported transport. */
    } /* Embedded input may still carry URL provenance without performing a startup request. */
#endif /* No hidden fallback or global provider registration is introduced. */
    if (fvm_desktop_playback_mailbox_create(&host.mailbox) != FVM_OK) return 1; /* Own one native mutex, no VM/window. */
    control.userdata = host.mailbox; /* Borrow the mailbox until the synchronous runner returns. */
    host.started = SDL_GetTicks(); /* Initialize the native clock before concurrent controller/owner reads. */
    thread = SDL_CreateThread(controller_thread, "FlashVM IDE controller", &host); /* Native controller only; owner stays here. */
    if (!thread) /* Preserve explicit disposal on native worker creation failure. */
    { /* No player run has started, but the native mailbox still owns a mutex. */
        fvm_desktop_playback_mailbox_release(host.mailbox); /* Release before SDK teardown. */
        fvm_desktop_shutdown(); /* End this example's otherwise unused SDK session. */
        return 1; /* Report capability/allocation failure without a detached thread. */
    } /* Both thread and mailbox remain owned until the common cleanup below. */
#if defined(FVM_DESKTOP_SOCKET_HTTP) || defined(FVM_DESKTOP_HTTP) /* Network builds bind cancellation before READY too. */
    { /* Provider and monitor descriptors remain in scope through synchronous return. */
        FVM_DesktopBackgroundLoader loader; /* Native byte transport is independent of controller commands. */
        FVM_DesktopLoadMonitor monitor; /* Owner observation/deadline/cancellation during fetch waits. */
        fvm_desktop_background_loader_init(&loader); /* Explicit copied provider defaults. */
        fvm_desktop_load_monitor_init(&monitor); /* Default asynchronous child/form publication. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* Use the selected native TCP/TLS implementation. */
        loader.transfer.fetch = fvm_desktop_socket_transfer_fetch; /* Worker-safe callback with default options. */
#else /* The alternate build uses the configured libcurl SDK. */
        loader.transfer.fetch = fvm_desktop_http_transfer_fetch; /* Thread-safety checks remain active. */
#endif /* Both transports deliver immutable bytes without entering VM. */
        loader.max_loads = EXAMPLE_MAILBOX_LOADS; /* Startup and any script requests share this bound. */
        monitor.timeout_ms = EXAMPLE_MAILBOX_LOAD_TIMEOUT_MS; /* Cooperative deadline, not a hard thread kill. */
        monitor.notify = fvm_desktop_playback_mailbox_load_notify; /* Reuse terminal stop during startup/waits. */
        monitor.userdata = host.mailbox; /* Same latch as playback control, never consumed by a load notice. */
        status = fvm_desktop_run_source_background_controlled(&options, &source, &loader, &monitor, &control, &error); /* Owner/UI loop. */
    } /* Loader/monitor storage is no longer borrowed after the runner has joined its jobs. */
#else /* An HTTP-free SDK supports the same controller with local/embedded movies. */
    status = fvm_desktop_run_source_controlled(&options, &source, 0, &control, &error); /* Synchronous owner loop. */
#endif /* Every path below signals completion, including failures before first READY. */
    SDL_AtomicSet(&host.done, 1); /* Wake a worker awaiting a snapshot that may never have existed. */
    SDL_WaitThread(thread, &worker_status); /* Join before any borrowed host/mailbox storage disappears. */
    fvm_desktop_playback_snapshot_init(&last); /* Prepare an independent exact-size final copy. */
    have_snapshot = fvm_desktop_playback_mailbox_poll(host.mailbox, &last) == FVM_OK; /* No VM is alive here. */
    if (status && error) fprintf(stderr, "%s\n", error); /* Static diagnostic is not freed by the caller. */
    if (!status && (worker_status || SDL_AtomicGet(&host.failed))) status = 1; /* Surface controller idle/protocol failure. */
    fvm_desktop_playback_mailbox_release(host.mailbox); /* No concurrent callback/post/poll remains after join. */
    if (fvm_desktop_shutdown() != FVM_OK) status = 1; /* No live mailbox/transfer/window may remain. */
    printf("completed=%lu\n", have_snapshot ? last.total_ticks : 0UL); /* The scalar/text copy survives mailbox release. */
    if (have_snapshot) fprintf(stderr, "copied_source=%s\n", last.source); /* Retaining text does not retain the old VM. */
    return status; /* The native host can now free its own borrowed input/provider storage. */
} /* No worker, managed pointer or native handle escapes the example's lifetime. */
