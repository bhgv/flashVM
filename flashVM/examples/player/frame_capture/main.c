/* Copy a chosen logical stage raster into native storage and write it after SDK cleanup; see README.md / README.ru.md. */
#include "../../../frontend/common/resource_frame.h" /* Bounded native frame cache and client metadata decoder. */
#include "resource_limits.h" /* Explicit twelve-MiB total pixel-cache budget for the optional example mode. */
#include "../../../frontend/common/resource_state.h" /* Copied timeline snapshots and independent observer events. */
#include "../../../frontend/common/resource_control.h" /* Native resource queue to player owner-control bridge. */
#include "../../../frontend/common/player.h" /* Public C89 observer/control/source declarations, with no SDL types. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* Use the provider selected by this SDK configuration. */
#include "../../../frontend/common/socket_http.h" /* Worker-safe native HTTP/TLS transport. */
#elif defined(FVM_DESKTOP_HTTP) /* The alternate build may use worker-safe libcurl. */
#include "../../../frontend/common/http.h" /* Explicit alternate provider, never a hidden fallback. */
#endif /* Local and linked-memory captures remain usable without HTTP support. */
#include <stdio.h> /* The example's output adapter writes ordinary binary PPM, independently of the library. */
#include <stdlib.h> /* Own and explicitly release a native pixel allocation. */
#include <string.h> /* Parse only this executable's small argument vocabulary. */
#include "movie.inc" /* Immutable linked SWF8 bytes, kept through the synchronous run. */
#define EXAMPLE_CAPTURE_MAX_TICK 1000UL /* Bound decimal input and the fallback per-root frame count. */
#define EXAMPLE_CAPTURE_LOADS 8UL /* Bound startup and child/form requests under the same provider policy. */
#define EXAMPLE_CAPTURE_TIMEOUT_MS 4000UL /* Cooperative total deadline per network resource. */
#define EXAMPLE_CAPTURE_RGB_CHANNELS 3U /* Binary PPM stores RGB only; its format has no alpha channel. */
#define EXAMPLE_CAPTURE_PPM_MAX 255U /* Eight-bit PPM component maximum, independent of native byte order. */

/* Host-owned native storage survives callbacks, VM/root disposal and SDK teardown. */
typedef struct Capture /* No managed context/root or borrowed frame/source pointer is retained. */
{ /* The main thread owns every field; this example has no concurrent pixel consumer. */
    unsigned long wanted; /* Requested run-wide host tick, immutable during playback. */
    unsigned long tick; /* Actual completed tick copied from the selected observation. */
    unsigned int width, height; /* Copied logical-stage pixel dimensions. */
    size_t stride; /* Byte distance between copied native rows. */
    unsigned char *pixels; /* [GC:] Native allocation, not a VM root; free explicitly after output. */
    FVM_RDispatch *dispatch; /* Optional owned native queue; no VM object or worker is allocated by it. */
    FVM_DesktopResourceControl resource; /* Borrowed adapter storage lives through unregister and runner return. */
    FVM_DesktopResourceState *state_adapter; /* Own native snapshot storage; never retain the callback's root/context. */
    FVM_RHandle state_target; /* Registered generation of the timeline observation endpoint. */
    FVM_RWatch state_watch; /* Own one local copied observation queue. */
    FVM_DesktopResourceSnapshot last_state; /* This scalar/source copy survives player teardown. */
    FVM_DesktopResourceFrame *frame_adapter; /* Own bounded native image buffers only in resource-frames mode. */
    FVM_RHandle frame_target; /* Registered frame resource incarnation, separate from timeline state. */
    FVM_RWatch frame_watch; /* Own one local metadata-event subscription; it does not pin pixels. */
    FVM_DesktopResourceFrameInfo last_frame; /* Owned metadata remains valid after SDK teardown. */
    int resource_frames; /* Optional export selector; ordinary capture/control keep their prior budgets. */
    int posted, control_failed; /* Track acknowledgement and cleanup/error status independently of captured pixels. */
    int captured; /* Set only after a complete validated row copy succeeds. */
} Capture; /* Caller stack storage outlives the synchronous observer/control callbacks. */

/* Copy only the requested completed tick while all frame-owned pointers remain borrowed. */
static FVM_Result capture_frame(const FVM_DesktopFrame *frame, void *raw) /* Native owner callback, no VM or window access. */
{ /* Defer file I/O until playback cleanup; keep the owner callback bounded by one allocation/copy. */
    Capture *capture = (Capture *)raw; /* Borrow the main function's native state. */
    FVM_Result result; /* Preserve a specific copy failure for the runner's ordinary failure path. */
    FVM_RResult exported; /* Distinguish successful export from bounded backpressure drops. */
    FVM_REvent event; /* Copy a metadata notification, never a borrowed image pointer. */
    if (capture->frame_adapter) { /* Export every presented frame independently of the selected PPM tick. */
        exported = fvm_desktop_resource_frame_capture(capture->frame_adapter, frame); /* Copy visible rows before this borrow ends. */
        if (exported != FVM_R_OK && exported != FVM_R_BUSY) return FVM_ERROR; /* Geometry/budget/publication errors fail this example explicitly. */
        if (exported == FVM_R_OK) { /* Only successful captures publish frame metadata. */
            if (fvm_rdispatch_event_take(capture->dispatch, 1, capture->frame_watch, &event) != FVM_R_OK || event.missed) return FVM_ERROR; /* Consume only this local observer. */
            if (fvm_desktop_resource_frame_read(event.data, event.size, &capture->last_frame) != FVM_R_OK) return FVM_ERROR; /* Decode a pointer-free metadata copy. */
        } /* BUSY leaves the last exported frame intact and increases the metadata drop counter. */
    } /* The runner still owns the original raster and continues normal frame selection. */
    if (capture->captured || frame->reason != FVM_DESKTOP_FRAME_TICK || frame->total_ticks < capture->wanted) /* Ignore other observations. */
        return FVM_OK; /* Refreshes and earlier host ticks do not fulfill the request. */
    capture->pixels = (unsigned char *)calloc(1, frame->byte_count); /* Own independent pixels; zero unused padding. */
    if (!capture->pixels) return FVM_OUT_OF_MEMORY; /* No retained borrowed pointer or partial success. */
    result = fvm_desktop_frame_copy(frame, capture->pixels, frame->byte_count, frame->stride); /* Validate both complete extents. */
    if (result != FVM_OK) /* Report bad geometry/capacity through the observer contract. */
    { /* Native ownership remains explicit on every error path. */
        free(capture->pixels); capture->pixels = 0; /* No output allocation survives an incomplete copy. */
        return result; /* The runner cleans its VM/window/jobs before returning failure. */
    } /* No borrowed frame pointer will be used after this function returns. */
    capture->width = frame->width; capture->height = frame->height; /* Copy dimensions as scalar values. */
    capture->stride = frame->stride; capture->tick = frame->total_ticks; /* Copy layout and completed progress. */
    capture->captured = 1; /* Owner control can now request ordinary successful STOP. */
    return FVM_OK; /* Drawing and its copied pixels are already committed. */
} /* frame/pixels/source from the runner cease to be borrowed here. */

/* Release native control registration only after its synchronous runner has returned. */
static void control_resources_release(Capture *capture) /* The queue never owns the Capture storage. */
{ /* A failure path may leave one accepted STOP awaiting acknowledgement. */
    FVM_RReply reply; /* Take copies bytes; no response pointer outlives dispatcher storage. */
    if (!capture->dispatch) return; /* Ordinary capture mode allocates no queue. */
    if (capture->posted == 1) /* Cancel queued work or acknowledge an already delivered reply. */
    { /* Running work cannot remain after the synchronous owner callback returned. */
        fvm_rdispatch_cancel(capture->dispatch, 1, 1); /* DONE may report CONFLICT; take still acknowledges it. */
        if (fvm_rdispatch_take(capture->dispatch, 1, 1, &reply) != FVM_R_OK) capture->control_failed = 1; /* Require no retained job. */
    } /* A previously acknowledged STOP needs no second take. */
    if (capture->frame_adapter && fvm_desktop_resource_frame_revoke(capture->frame_adapter, 1) != FVM_R_OK) capture->control_failed = 1; /* Drop this session's watches and any retained leases. */
    if (capture->frame_adapter && fvm_desktop_resource_frame_destroy(&capture->frame_adapter) != FVM_R_OK) capture->control_failed = 1; /* Free native pixels before dispatcher teardown. */
    if (capture->state_adapter && fvm_desktop_resource_state_destroy(&capture->state_adapter) != FVM_R_OK) capture->control_failed = 1; /* Close state watches before queue destruction. */
    if (capture->resource.struct_size && fvm_rdispatch_unregister(capture->dispatch, capture->resource.target) != FVM_R_OK) capture->control_failed = 1; /* Release borrow. */
    if (fvm_rdispatch_destroy(&capture->dispatch) != FVM_R_OK) capture->control_failed = 1; /* No host pixels or userdata are freed by destroy. */
} /* SDK shutdown follows this native resource cleanup. */
/* Create an optional local resource endpoint with explicit control authority and no transport. */
static int control_resources_setup(Capture *capture) /* Single-owner mode deliberately needs no mutex policy. */
{ /* A real producer thread supplies working mutex/owner hooks instead. */
    if (fvm_rdispatch_create(0, &capture->dispatch) == FVM_R_OK && /* Fixed bounded native allocation. */
        fvm_desktop_resource_control_init(&capture->resource, capture->dispatch, "/timeline/ctl") == FVM_R_OK && /* Copied path/operations. */
        fvm_rdispatch_grant(capture->dispatch, capture->resource.target, 1, FVM_R_CONTROL) == FVM_R_OK && /* Only this local controller may stop playback. */
        fvm_desktop_resource_state_create(capture->dispatch, "/timeline/state", &capture->state_adapter, &capture->state_target) == FVM_R_OK && /* Own a copied observation endpoint. */
        fvm_rdispatch_grant(capture->dispatch, capture->state_target, 1, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK && /* Explicit observation authority. */
        fvm_rdispatch_subscribe(capture->dispatch, 1, capture->state_target, &capture->state_watch) == FVM_R_OK) { /* The state endpoint is ready before optional image allocation. */
        if (!capture->resource_frames) return 1; /* Preserve the existing resource-control memory budget. */
        if (fvm_desktop_resource_frame_create(capture->dispatch, "/display/frame", EXAMPLE_CAPTURE_FRAME_CAPACITY, &capture->frame_adapter, &capture->frame_target) == FVM_R_OK && /* Allocate three bounded native slots. */
            fvm_rdispatch_grant(capture->dispatch, capture->frame_target, 1, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK && /* Host decides pixel visibility. */
            fvm_rdispatch_subscribe(capture->dispatch, 1, capture->frame_target, &capture->frame_watch) == FVM_R_OK) return 1; /* Events do not themselves acquire leases. */
    } /* Trusted local session only. */
    control_resources_release(capture); /* Dispose any partially acquired registration/allocation. */
    return 0; /* No VM, window or output file was opened. */
} /* Registry/adapter storage remains owned by main. */
/* Stop after capture directly or through the optional copied native resource queue. */
static FVM_DesktopPlaybackCommand stop_after_capture(const FVM_DesktopPlaybackState *state, void *raw) /* Same owner thread. */
{ /* No context/root from state is retained or sent to the native queue. */
    Capture *capture = (Capture *)raw; /* Shared only with the synchronous frame observer. */
    FVM_RRequest request; /* Native request descriptor; no S-record encoding is required for local C calls. */
    FVM_RReply reply; /* Owned response copy used for delivery acknowledgement. */
    FVM_REvent event; /* Own the publication bytes only through this bounded native copy. */
    FVM_RResult observed; /* Distinguish quiet turns from errors. */
    FVM_DesktopPlaybackCommand command; /* Player applies this returned policy intent after callback return. */
    if (!capture->dispatch) return capture->captured ? FVM_DESKTOP_PLAYBACK_STOP : FVM_DESKTOP_PLAYBACK_KEEP; /* Existing default behavior. */
    if (fvm_desktop_resource_state_observe(capture->state_adapter, state) != FVM_R_OK) { capture->control_failed = 1; return FVM_DESKTOP_PLAYBACK_STOP; } /* Copy before the next command takes effect. */
    observed = fvm_rdispatch_event_take(capture->dispatch, 1, capture->state_watch, &event); /* Only this reader's queue is consumed. */
    if (observed == FVM_R_OK) { /* READY/TICK and changed policy carry independent snapshot bytes. */
        if (fvm_desktop_resource_state_read(event.data, event.size, &capture->last_state) != FVM_R_OK || event.missed) { capture->control_failed = 1; return FVM_DESKTOP_PLAYBACK_STOP; } /* Decode without any VM pointer. */
    } else if (observed != FVM_R_PENDING) { capture->control_failed = 1; return FVM_DESKTOP_PLAYBACK_STOP; } /* Ordinary quiet turns publish nothing. */
    if (capture->captured && !capture->posted) /* Submit exactly one terminal request after the pixel copy. */
    { /* The target/operation, not a caller-provided right, selects CONTROL permission. */
        memset(&request, 0, sizeof(request)); request.session = 1; request.id = 1; /* Trusted local identity; no reconnect protocol here. */
        request.target = capture->resource.target; request.operation = FVM_DESKTOP_RESOURCE_STOP; /* Empty typed command payload. */
        if (fvm_rdispatch_post(capture->dispatch, &request) != FVM_R_OK) /* Acceptance is separate from execution. */
        { /* Preserve failure instead of writing a misleading successful output. */
            capture->control_failed = 1; return FVM_DESKTOP_PLAYBACK_STOP; /* Runner still performs normal joined cleanup. */
        } /* No borrowed stack request pointer escapes post. */
        capture->posted = 1; /* Retain identity until the owner delivery reply is taken. */
    } /* Before capture, notify normally returns KEEP with an empty queue. */
    command = fvm_desktop_resource_control_notify(state, &capture->resource); /* At most one queued job, on the playback owner. */
    if (capture->posted == 1 && command == FVM_DESKTOP_PLAYBACK_STOP) /* This marks command delivery, not an extra rendered frame. */
    { /* The requested frame was already copied before STOP was posted. */
        if (fvm_rdispatch_take(capture->dispatch, 1, 1, &reply) != FVM_R_OK || reply.result != FVM_R_OK || reply.size) capture->control_failed = 1; /* Acknowledge safely. */
        else capture->posted = 2; /* Queue capacity and duplicate-request retention are released. */
    } /* Frame completion was observed separately in capture_frame. */
    return command; /* Native player policy applies STOP after this function returns. */
} /* No VM pointer or dispatcher-owned borrow is retained. */

/* Another host may instead combine its own controller mailbox with the frame observer. */

/* Write visible RGB after all runner resources are gone; native RGBA remains independently owned. */
static int write_ppm(const char *path, const Capture *capture) /* Output policy belongs to this console adapter, not the SDK. */
{ /* Binary mode preserves byte counts on Windows as well as POSIX hosts. */
    FILE *file = fopen(path, "wb"); /* Use the native C runtime's output filename convention. */
    unsigned char row[FVM_DESKTOP_FRAME_MAX_DIMENSION * EXAMPLE_CAPTURE_RGB_CHANNELS]; /* Bounded temporary RGB row. */
    unsigned int x, y; /* C89 loop cursors declared before statements. */
    int ok; /* Preserve either header/body/close failure without leaking the FILE handle. */
    if (!file) return 0; /* No image was opened for output. */
    ok = fprintf(file, "P6\n%u %u\n%u\n", capture->width, capture->height, EXAMPLE_CAPTURE_PPM_MAX) > 0; /* Standard PPM header. */
    for (y = 0; ok && y < capture->height; ++y) /* Convert exactly each visible native row. */
    { /* Padding and alpha are excluded from this RGB-only file format. */
        const unsigned char *source = capture->pixels + (size_t)y * capture->stride; /* Independent buffer, not borrowed VM memory. */
        for (x = 0; x < capture->width; ++x) /* Preserve explicit R,G,B byte order. */
            memcpy(row + x * EXAMPLE_CAPTURE_RGB_CHANNELS, source + x * FVM_DESKTOP_FRAME_PIXEL_BYTES, EXAMPLE_CAPTURE_RGB_CHANNELS); /* Drop alpha only. */
        ok = fwrite(row, EXAMPLE_CAPTURE_RGB_CHANNELS, capture->width, file) == capture->width; /* One bounded write per row. */
    } /* A failed write stops further output but still closes the file. */
    if (fclose(file)) ok = 0; /* Flush/close failures are real output failures. */
    return ok; /* A failed output may leave a partial file; the SDK itself wrote no file. */
} /* Native RGBA is still owned by main and can be freed afterwards. */

/* Parse one complete positive decimal without locale, signs, partial acceptance or overflow. */
static int parse_tick(const char *text, unsigned long *out) /* Library callers assign descriptors directly instead. */
{ /* Reject malformed arguments before any SDK or output file is opened. */
    unsigned long value = 0; /* Zero is not a capture-before-first-tick selector. */
    if (!text || !*text) return 0; /* Require at least one digit. */
    while (*text) /* Consume the complete argument. */
    { /* Bound each multiplication on every supported CPU width. */
        unsigned long digit; /* C89 declaration precedes the block's statements. */
        if (*text < '0' || *text > '9') return 0; /* Signs/whitespace/nondecimal text are invalid. */
        digit = (unsigned long)(*text++ - '0'); /* Convert a known ASCII decimal digit. */
        if (value > (EXAMPLE_CAPTURE_MAX_TICK - digit) / 10UL) return 0; /* Check before arithmetic can overflow. */
        value = value * 10UL + digit; /* The result is within the declared capture bound. */
    } /* No trailing bytes remain. */
    if (!value) return 0; /* One or more host advances are required. */
    *out = value; /* Publish only a valid complete number. */
    return 1; /* No allocation, rendering or file I/O occurred. */
} /* CLI policy remains separate from the C runner and copy utility. */

/* Main/UI owner adapts argv to typed policies, then consumes its own copy after runner teardown. */
int main(int argc, char **argv) /* A native IDE can replace file output and console parsing. */
{ /* Keep every borrowed input descriptor/string alive until synchronous return. */
    Capture capture; /* Native owned pixel storage and scalar completion state. */
    FVM_DesktopOptions options; /* Playback settings, initialized separately. */
    FVM_DesktopSource source; /* File/URL or immutable linked-memory source with explicit provenance. */
    FVM_DesktopPlaybackControl control; /* Successful owner STOP once the desired raster is copied. */
    FVM_DesktopFrameObserver observer; /* Separate additive pixel callback policy. */
    const char *error = 0; /* Borrowed static diagnostic text, never freed by this caller. */
    int embedded = 0, offscreen = 0, resource_mode = 0, tick_selected = 0; /* Reject repeated console selectors. */
    int valid = argc >= 3, argument, status; /* Require source and output filename after optional arguments. */
    memset(&capture, 0, sizeof(capture)); capture.wanted = 1; /* Default selects the first successful tick raster. */
    for (argument = 1; argument < argc - 2; ++argument) /* Leave exactly source and output positional arguments. */
    { /* The shared library never needs this argv vocabulary. */
        if (!strcmp(argv[argument], "--embedded") && !embedded) embedded = 1; /* Select linked bytes once. */
        else if (!strcmp(argv[argument], "--offscreen") && !offscreen) offscreen = 1; /* Select timer-only delivery once. */
        else if (!strcmp(argv[argument], "--resource-control") && !resource_mode) resource_mode = 1; /* Optional native resource queue. */
        else if (!strcmp(argv[argument], "--resource-frames") && !resource_mode) { resource_mode = 1; capture.resource_frames = 1; } /* Frame export also enables ordinary native resource control. */
        else if (!strcmp(argv[argument], "--tick") && !tick_selected && argument + 1 < argc - 2) /* Require a numeric value. */
        { /* Consume one complete bounded tick count. */
            tick_selected = 1; /* A second selector is an argument error. */
            valid = parse_tick(argv[++argument], &capture.wanted) && valid; /* Preserve prior parse failure. */
        } /* Every other option must fail before SDK/output startup. */
        else valid = 0; /* Unknown, duplicate or incomplete argument. */
    } /* Final positional storage remains owned by this process through playback/output. */
    if (!valid || argv[argc - 2][0] == '-' || argv[argc - 1][0] == '-') /* Avoid ambiguous missing/unknown options. */
    { /* Show this adapter's complete syntax and leave the filesystem untouched. */
        fprintf(stderr, "Usage: %s [--embedded] [--offscreen] [--resource-control | --resource-frames] [--tick 1..1000] file-or-http[s]-URL output.ppm\n", argv[0]); /* Explicit image destination. */
        return 2; /* Argument failure without any native resources or output file. */
    } /* Source/provider validation is delegated to the ordinary typed API. */
    fvm_desktop_options_init(&options); /* Initialize every scalar playback option. */
    options.mode = FVM_EXECUTION_INTERPRETER; options.mute = 1; options.stats = 1; /* No JIT/audio device prerequisite. */
    options.frames = EXAMPLE_CAPTURE_MAX_TICK + 1UL; /* Fallback bound; owner STOP normally ends earlier. */
    fvm_desktop_playback_control_init(&control); /* Running mode, exact ABI size and no hidden registration. */
    control.notify = stop_after_capture; control.userdata = &capture; /* Same-owner scalar coordination needs no worker mutex. */
    fvm_desktop_frame_observer_init(&observer); /* Initialize exact observer descriptor size. */
    observer.notify = capture_frame; observer.userdata = &capture; /* Borrow native state through synchronous return. */
    source.location = argv[argc - 2]; /* Explicit local/network source provenance for linked bytes too. */
    source.kind = strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE; /* Choose startup I/O. */
    if (embedded) source.kind = FVM_DESKTOP_SOURCE_MEMORY; /* No startup read; retain the explicit origin. */
    source.bytes = embedded ? embedded_movie : 0; source.size = embedded ? sizeof(embedded_movie) : 0; /* Count binary data, never strlen. */
#if !defined(FVM_DESKTOP_SOCKET_HTTP) && !defined(FVM_DESKTOP_HTTP) /* Diagnose a missing optional capability before any startup. */
    if (source.kind == FVM_DESKTOP_SOURCE_URL) /* No provider is configured for an initial URL fetch. */
    { /* Local or embedded pixels remain available in the same lean SDK. */
        fprintf(stderr, "URL loading requires a configured HTTP provider\n"); /* Explicit host capability error. */
        return 1; /* Output file and native player resources remain unopened. */
    } /* Embedded input may retain URL provenance without reading startup bytes. */
#endif /* A custom host can instead provide its own compatible native transport. */
    if (resource_mode && !control_resources_setup(&capture)) /* No output/SDK startup occurred yet. */
    { /* Setup cleans every partially acquired native resource. */
        fprintf(stderr, "resource control setup failed\n"); return 1; /* Do not start an uncontrolled substitute run. */
    } /* Capture/adapter storage remains alive through synchronous runner return. */
#if defined(FVM_DESKTOP_SOCKET_HTTP) || defined(FVM_DESKTOP_HTTP) /* Explicit optional worker transport for startup/child requests. */
    { /* Provider and monitor storage remain borrowed through joined runner cleanup. */
        FVM_DesktopBackgroundLoader loader; /* Byte transport, independent of the native frame observer. */
        FVM_DesktopLoadMonitor monitor; /* Owner progress/cancellation/deadline policy. */
        fvm_desktop_background_loader_init(&loader); fvm_desktop_load_monitor_init(&monitor); /* No workers or I/O yet. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* Select the configured native socket provider. */
        loader.transfer.fetch = fvm_desktop_socket_transfer_fetch; /* Worker-safe native HTTP/TLS. */
#else /* The alternate SDK uses libcurl with its own global thread-safety check. */
        loader.transfer.fetch = fvm_desktop_http_transfer_fetch; /* Worker fetches bytes, never scene pixels. */
#endif /* No implicit provider substitution occurs. */
        loader.max_loads = EXAMPLE_CAPTURE_LOADS; monitor.timeout_ms = EXAMPLE_CAPTURE_TIMEOUT_MS; /* Explicit finite resource policy. */
        if (offscreen) /* No video/window/event queue ownership is acquired. */
            status = fvm_desktop_run_source_background_offscreen(&options, &source, &loader, &monitor, &control, &observer, &error); /* Explicit provider, same copied pixels. */
        else /* Native presentation remains the default for existing console commands. */
            status = fvm_desktop_run_source_background_observed(&options, &source, &loader, &monitor, &control, &observer, &error); /* Owner loop. */
    } /* No callback retains loader/monitor stack storage after the runner returns. */
#else /* HTTP-free SDK keeps local/embedded capture and deferred local resources available. */
    { /* An ordinary loader permits bounded local child/form requests. */
        FVM_DesktopLoader loader; /* No network provider or SDL types are needed in this policy. */
        fvm_desktop_loader_init(&loader); loader.max_loads = EXAMPLE_CAPTURE_LOADS; /* Explicit resource budget. */
        if (offscreen) /* Timer-only local/linked execution requires no working video driver. */
            status = fvm_desktop_run_source_offscreen(&options, &source, &loader, &control, &observer, &error); /* Borrow source only through return. */
        else /* Keep the existing windowed tutorial route. */
            status = fvm_desktop_run_source_observed(&options, &source, &loader, &control, &observer, &error); /* URL reports unsupported transport. */
    } /* All borrowed source bytes remain owned until this synchronous return. */
#endif /* Output happens only after player-owned resources have been disposed. */
    control_resources_release(&capture); /* Release registration/requests before SDK shutdown or Capture disposal. */
    if (capture.control_failed) status = 1; /* Native acknowledgement/cleanup failure is not successful capture. */
    if (status && error) fprintf(stderr, "%s\n", error); /* Report static text without freeing it. */
    if (fvm_desktop_shutdown() != FVM_OK) status = 1; /* Require complete VM/window/transfer/session cleanup. */
    if (!status && !capture.captured) /* A user close before the desired tick did not fulfill this capture request. */
    { /* Do not publish an empty or misleading image. */
        fprintf(stderr, "requested tick was not captured\n"); status = 1; /* Output filename has not been opened. */
    } /* Only the independent native copy is used beyond this point. */
    if (!status && !write_ppm(argv[argc - 1], &capture)) /* Image output is separate from playback success. */
    { /* Handle ordinary I/O failure without leaking copied pixels. */
        fprintf(stderr, "PPM output failed\n"); status = 1; /* A failed write may leave a partial example file. */
    } /* This example's PPM drops alpha; the public copied buffer still contained all RGBA channels. */
    if (!status) printf("captured_tick=%lu width=%u height=%u\n", capture.tick, capture.width, capture.height); /* Copied scalars only. */
    free(capture.pixels); /* [GC:] Release native bytes explicitly; no VM/pin is retained by the output adapter. */
    return status; /* No borrowed frame, worker, window or owned pixel allocation escapes. */
} /* Other C hosts can replace the PPM adapter with their own encoder/renderer or asynchronous copied-buffer handoff. */
