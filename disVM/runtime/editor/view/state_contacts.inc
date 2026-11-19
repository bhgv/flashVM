/* Copyright (c) 2026 bhgv
 * EN: Renderer state and native contact bookkeeping; included by the owner translation unit.
 * RU: Состояние renderer и учёт нативных контактов; включается в translation unit владельца.
 */
#define EDITOR_WIDTH 960U                /* Initial viewport width in pixels. */
#define EDITOR_HEIGHT 640U               /* Initial viewport height in pixels. */
#define EDITOR_SOURCE_MAX 4194304U       /* Maximum assembled UTF-8 document snapshot. */
#define EDITOR_VIEW_BYTES 4096U          /* Bounded first-page TextField preview. */
#define EDITOR_REPLY_META 56U            /* Seven canonical eight-character hexadecimal words. */
#define EDITOR_SELECTION 65536UL         /* Dis selection notification code. */
#define EDITOR_ZOOM 65547UL              /* Dis-admitted font-size step proposal. */
#define EDITOR_STYLE 65548UL             /* Complete-view font-size and row geometry. */
#define EDITOR_PINCH 65550UL             /* Normalized two-finger pinch input code. */
#define EDITOR_LEADING 2.0               /* Explicit TextFormat leading shared by all editor geometry. */
#define EDITOR_FONT_MIN 6                /* Bounded minimum editor pixel size. */
#define EDITOR_FONT_MAX 48               /* Bounded maximum editor pixel size. */
#define EDITOR_PINCH_THRESHOLD 0.015     /* Normalized SDL distance change per admitted zoom step. */
#define EDITOR_PANEL_BROWSER 9U          /* Read-only selected directory-entry presentation. */
#define EDITOR_PANEL_EXPORT_PATH 11U     /* Editable explicit artifact destination. */
#define EDITOR_PANEL_BROWSE_PATH 10U     /* Editable directory path, with caret and normal field geometry. */
#define EDITOR_PANEL 65543UL             /* Dis-owned prompt mode, selection and counted field text. */
#define EDITOR_INK_BATCH_MS 4U           /* Native live-ink coalescing bound; no SWF-frame dependency. */
#define EDITOR_KEYBOARD_BATCH_MS 16U     /* Owner-poll batching budget, independent of SWF timeline frequency. */
#define EDITOR_VIEWPORT 65540UL          /* Dis-owned Unicode window and monotonic view ticket. */
#define EDITOR_SCROLL 65541UL            /* Ticket/revision-bound logical scroll request. */
#define EDITOR_PLACE 65542UL             /* Ticket/revision-bound glyph hit proposal. */
#define EDITOR_RESIZE 65552UL            /* Raw/checked width-height proposal, bound to the completed view. */
#define EDITOR_CANVAS 65553UL            /* Dis-admitted pixel extent before Style and snapshot frames. */
#define EDITOR_WIDTH_MIN 240U            /* Bounded minimum native surface width. */
#define EDITOR_HEIGHT_MIN 160U           /* Bounded minimum native surface height. */
#define EDITOR_EXTENT_MAX 2048U          /* Bound each dimension and the RGBA allocation to 16 MiB. */
#define EDITOR_GUTTER 65563UL            /* Dis-owned line-number visibility, copied per completed view. */
#define EDITOR_QUIT 65539UL              /* Dis controller's dirty-checked close operation. */

#define EDITOR_CLIP_FIELD_WRITE 65561UL  /* Copy Dis-authorized modal field bytes, excluding the label. */
#define EDITOR_FIELD_PLACE 65562UL       /* Checked modal scalar placement/drag proposal from native glyphs. */
#define EDITOR_PANEL_SELECTION 65560UL   /* Dis-owned modal caret/anchor, independent of the source selection. */
#define EDITOR_CLIP_WRITE 65554UL        /* Dis-authorized source selection copied to native clipboard. */
#define EDITOR_CLIP_READ 65556UL         /* Dis request for one immutable native clipboard snapshot. */
#define EDITOR_CLIP_DATA 65557UL         /* Revision/transfer-bound ordered paste fragment. */
#define EDITOR_CLIP_CONTINUE 65559UL     /* One fragment credit without retransmitting an unchanged source view. */
#define EDITOR_CLIP_ACK 65558UL          /* Acknowledged clipboard write or explicit read failure. */
#define EDITOR_CLIP_MAX 65536U           /* Bound each clipboard snapshot and staged paste in UTF-8 bytes. */
#define EDITOR_CLIP_OK 0                 /* Successful copied-value result, matching Idecore. */
#define EDITOR_CLIP_TOO_LARGE 3          /* UTF-8 clipboard bound rejected before any source edit. */
#define EDITOR_CLIP_INVALID 4            /* Native API, allocation, NUL or UTF-8 admission failure. */
#define EDITOR_CLIP_PREFIX 8U            /* Original completed-view ticket before each Unicode body. */

static char *clipboard_text;             /* Owned replay clipboard value; also retains successful native writes. */
static char *clipboard_transfer;         /* Owned immutable UTF-8 read snapshot until its last copied fragment. */
static size_t clipboard_total;           /* Staged clipboard bytes, at most EDITOR_CLIP_MAX. */
static size_t clipboard_offset;          /* Next byte offset to emit, never a Unicode source coordinate. */
static unsigned long clipboard_ticket;   /* Original view identity retained across fragment acknowledgements. */
static unsigned long clipboard_revision; /* Source revision captured by the Dis-authorized native command. */
static int clipboard_ack;                /* One pending acknowledgement; zero means none. */
static int clipboard_status;             /* Copied Idecore status: 0 success, 3 too large, 4 invalid/native error. */

static unsigned int canvas_width = EDITOR_WIDTH;   /* Last Dis-admitted raster/field width in pixels. */
static unsigned int canvas_height = EDITOR_HEIGHT; /* Last Dis-admitted raster/field height in pixels. */
static unsigned long canvas_revision;              /* Expected document revision for the pending canvas. */
static int desktop_bound;              /* Context-local desktop binding is retired before context destruction. */
static const char *font_path;          /* Borrowed explicit TTF/OpenType command-line path. */
static char *default_font_path;        /* Owned relocatable sibling resource path, unless --font overrides. */
static unsigned char *font_bytes;      /* Owned immutable face bytes for future actual size changes. */
static size_t font_byte_count;         /* Active font byte extent, bounded during admission. */
static int font_size = 11;             /* Initial pixel size; subsequent policy comes from Dis Style. */
static unsigned long style_revision;   /* Expected content revision of the pending font style. */
static double pinch_distance;          /* Bounded native gesture accumulator, never a document coordinate. */
static const char *document_path = ""; /* Borrowed configured source/destination, never guessed. */
static const char *event_path;         /* Optional deterministic counted-envelope input. */
static const char *capture_path;       /* Optional final PPM capture destination. */
static int new_document;               /* Explicit --new selects a destination without opening it. */
static FILE *events;                   /* Owned replay stream; no deployed Python dependency. */
static FVM_Runtime *runtime;           /* Owner-local FlashVM runtime. */
static FVM_Context *context;           /* Owner-local context; no pointer enters Dis. */
static FVM_Font *layout_font;          /* Owned same-face layout for Dis selection geometry. */
static long caret;                     /* Last Dis Unicode scalar caret, presentation only. */
static long anchor;                    /* Last Dis Unicode scalar selection anchor. */
static FVM_Object *panel_field;        /* [GC:] Pinned display-only field; input policy remains in Dis. */
static unsigned long
    panel_mode; /* Dis panel mode: 0 file, 1..6 edit, 7 inspect, 8 close, 9 browser, 10 directory path. */
static unsigned long panel_revision;           /* Expected content revision for the pending panel. */
static long panel_caret;                       /* Copied Unicode field caret, excluding its native label prefix. */
static long panel_anchor;                      /* Copied Unicode field selection anchor. */
static unsigned long panel_selection_revision; /* Expected source revision of the pending field endpoints. */
static FVM_TextLayout *panel_layout;           /* Owned same-contract glyph layout for the current completed field. */
static double panel_horizontal;                /* Effective native field scroll, never a source/text authority. */
static size_t panel_prefix;                    /* ASCII label byte/scalar extent used only for geometry. */
static int panel_selected;                     /* Dis-owned field select-all presentation flag. */
static char panel_text[1024];                  /* Copied counted UTF-8 prompt/path label. */
static size_t panel_size;                      /* Active panel bytes, never implicitly strlen-decoded. */
static FVM_Object *field;                      /* [GC:] Pinned display object until destroy. */
static int pixels_valid;                    /* Owner buffer contains the last successfully displayed complete frame. */
static unsigned char *pixels;               /* Owned RGBA8 raster storage. */
static FVM_RenderQueue *raster_queue;       /* Owned persistent native raster worker, joined before VM teardown. */
static DVM_EditorFrame *displayed_frame;    /* Owned completed baseline; workers never retain this frame. */
static const DVM_EditorFrame *upload_frame; /* Borrowed completed damage during SDL presentation only. */
static DVM_EditorFrame *recording_frame;    /* Owner-only frame builder; workers never read this global. */
static int desktop_pan;                     /* Latched middle/Ctrl+Shift drag; copied table gesture only. */
static unsigned long ink_token;             /* Sole native live trajectory; never a Dis reference. */
static unsigned long vector_drag;           /* Captured logical canvas identity; never a VM object. */
#ifdef DVM_EDITOR_SDL
static SDL_Event local_pointer; /* Copied native pointer identity for bound geometry motion. */
static int local_pointer_valid; /* Identity exists until release, cancellation or lifecycle barrier. */
#endif
static int header_drag;            /* Owner-side capture: only continuing header motion may bypass pixel readiness. */
static int raster_pending;         /* Owner-only pointer geometry barrier until the latest frame is presented. */
static FVM_Object *paint_callback; /* [GC:] Pinned owner-only frame-free notification. */
static int paint_due;              /* Frame-free notification delivered, latest state may be prepared. */
static int desktop_dirty;          /* Latest complete desktop snapshot awaits owner-side painting. */
#ifdef DVM_EDITOR_SDL
static SDL_Event ink_pointer;      /* Copied source identity of the sole native ink capture. */
static SDL_Event postponed_event;  /* One FIFO barrier behind a synthetic ink release. */
static int postponed_valid;        /* No later SDL input may overtake the saved event. */
static Uint32 desktop_animation_clock;  /* Last native visual-clock sample; independent of SWF frames. */
static int desktop_animation_clock_set; /* Initial sample is not an elapsed delta. */
static Uint32 desktop_dirty_since;      /* First unpainted snapshot time; unsigned elapsed time tolerates wrap. */
#endif

static char *snapshot;                /* Owned assembly storage, independent of both VM heaps. */
static size_t assembled;              /* Next expected UTF-8 byte offset. */
static size_t extent;                 /* Total expected snapshot bytes. */
static unsigned long revision;        /* Dis revision of the pending/published snapshot. */
static unsigned long status;          /* Dis status published with that snapshot. */
static long saved;                    /* Dis saved revision, used only for display. */
static unsigned long view_ticket;     /* Monotonic Dis view identity, independent of content revision. */
static unsigned long view_revision;   /* Expected content revision for viewport metadata. */
static unsigned long view_total;      /* Expected document Unicode scalar count. */
static int initial_line_numbers = 1;  /* Startup preference only, read once by Dis. */
static int line_numbers;              /* Admitted Dis view preference; no native shortcut changes it. */
static unsigned long gutter_revision; /* Revision that authorizes this view's gutter state. */
static unsigned int gutter_width;     /* Measured native pixels reserved beside source text. */
static unsigned long view_top;        /* Zero-based logical top line chosen by Dis. */
static long view_first;               /* Dis-selected source scalar at display start. */
static long view_last;                /* Dis-selected source scalar after display end. */
static size_t display_start;          /* Native byte translation of the admitted source scalar start. */
static size_t display_size;           /* Current display UTF-8 byte length, at most 4096. */
static FVM_TextLayout *hit_layout;    /* Owned actual glyph layout for the completed view. */
static double horizontal;             /* Renderer pixel translation, never a document coordinate. */
static int complete;                  /* A selection frame completed the current view. */
static int system_clipboard_events;   /* Explicit SDL clipboard replay check; ordinary replay remains isolated. */
static int window_replay;             /* Explicit SDL upload check with deterministic input. */
static int assembling;                /* A document snapshot is being assembled. */
#ifdef DVM_EDITOR_SDL
static SDL_Window *window;            /* Main-thread-only native window. */
static SDL_Renderer *renderer;        /* Main-thread presentation owner. */
static SDL_Texture *texture;          /* RGBA8 upload target. */
#endif


#ifdef DVM_EDITOR_SDL


/* Keep the latest consecutive absolute motion before painting. event belongs
 * to the SDL/main owner. Bound work to 256 updates and stop before every key,
 * button, focus or contact-identity boundary; releases must remain queued. */


/* Track bounded native contacts and emit only copied gesture proposals. The
 * Limbo layout owner still admits tickets, window hits and final geometry.
 * Return one for a complete proposal, zero for ignored/incomplete contacts. */
static int desktop_touch(const SDL_TouchFingerEvent *event, DVM_UIMessage *message)
{
    static SDL_TouchID devices[8]; /* Distinguish equal finger IDs on different devices. */
    static SDL_FingerID ids[8];    /* Native IDs remain on the SDL owner. */
    static double xs[8];           /* Normalized native positions, never source offsets. */
    static double ys[8];           /* Normalized native positions, never source offsets. */
    static unsigned char used[8];  /* Bounded live contact slots. */
    static unsigned int gesture;   /* Active contact count, zero before begin. */
    static double span;            /* Previous two-contact distance for pinch steps. */
    static int tap_eligible;       /* Motion and contact transitions invalidate a tap. */
    static double down_x;          /* Single-contact begin point in normalized coordinates. */
    static double down_y;          /* Single-contact begin point in normalized coordinates. */
    static Uint32 tap_time;        /* Last completed native single-contact tap timestamp. */
    static double tap_x;           /* Last tap canvas position, checked for spatial proximity. */
    static double tap_y;           /* Last tap canvas position, checked for spatial proximity. */
    static unsigned int session_contacts; /* Max contacts in this session; no phantom downgrade taps. */
    static double first_x;        /* First contact of the session selects the gesture surface. */
    static double first_y;        /* Preserved until every finger has left. */
    static double triple_x;       /* Centroid reference for subsequent three-finger deltas. */
    static double triple_y;       /* No later finger can change the selected surface. */
    static int zoom_pending;       /* Ensure the final contact release ends transient zoom paint. */
    unsigned int index;
    unsigned int slot = 8;
    unsigned int count = 0;
    unsigned int first = 8;
    unsigned int second = 8;
    double x = 0;
    double y = 0;
    double oldx = 0;
    double oldy = 0;
    unsigned int oldcount = 0;
    if (!event)
    {
        memset(used, 0, sizeof(used));
        gesture = 0;
        session_contacts = 0;
        span = 0;
        zoom_pending = 0;
        tap_time = 0;
        return 0;
    }
    if (!isfinite(event->x) || !isfinite(event->y) || event->x < 0 || event->x > 1 || event->y < 0 || event->y > 1)
    {
        return 0;
    }
    for (index = 0; index < 8; ++index)
    {
        if (used[index])
        {
            oldx += xs[index];
            oldy += ys[index];
            ++oldcount;
            if (ids[index] == event->fingerId && devices[index] == event->touchId)
            {
                slot = index;
            }
        }
    }
    if (slot == 8 && event->type == SDL_FINGERDOWN)
    {
        for (index = 0; index < 8; ++index)
        {
            if (!used[index])
            {
                slot = index;
                ids[index] = event->fingerId;
                devices[index] = event->touchId;
                break;
            }
        }
    }
    if (slot == 8)
    {
        return 0;
    }
    if (oldcount == 0 && event->type == SDL_FINGERDOWN)
    {
        first_x = event->x;
        first_y = event->y;
        session_contacts = 1;
    }
    xs[slot] = event->x;
    ys[slot] = event->y;
    used[slot] = event->type != SDL_FINGERUP;
    for (index = 0; index < 8; ++index)
    {
        if (used[index])
        {
            x += xs[index];
            y += ys[index];
            if (first == 8)
            {
                first = index;
            }
            else if (second == 8)
            {
                second = index;
            }
            ++count;
        }
    }
    memset(message, 0, sizeof(*message));
    if (count > session_contacts)
    {
        session_contacts = count;
    }
    message->kind = DVM_UI_POINTER;
    if ((gesture && count != gesture) || (zoom_pending && event->type == SDL_FINGERUP))
    {
        /* A second finger cancels a provisional tap; it must not activate
         * a shelf button before the third contact can start its pan. */
        message->code = gesture == 3 ? 7 : (gesture == 1 && count > 1 ? 11 : (gesture == 1 ? 1 : 6));
        zoom_pending = 0;
        if (count > 1)
        {
            tap_time = 0;
            tap_eligible = 0;
        }
        if (gesture == 1 && session_contacts == 1 && tap_eligible && event->type == SDL_FINGERUP && count == 0 &&
            (dvm_desktop_header(event->x * canvas_width, event->y * canvas_height) ||
             dvm_desktop_zoom_button(event->x * canvas_width, event->y * canvas_height)))
        {
            tap_time = event->timestamp;
            tap_x = event->x * canvas_width;
            tap_y = event->y * canvas_height;
        }
        message->flags = 3;
        message->first = (long)((gesture == 3 ? first_x + oldx / oldcount - triple_x : oldx / oldcount) * canvas_width * 20);
        message->second = (long)((gesture == 3 ? first_y + oldy / oldcount - triple_y : oldy / oldcount) * canvas_height * 20);
        gesture = 0;
        span = 0;
        return 1;
    }
    if (session_contacts > count && !gesture)
    {
        return 0; /* A completed multi-contact gesture cannot become a new tap while lifting. */
    }
    if (count == 1)
    {
        if (!gesture)
        {
            tap_eligible = 1;
            down_x = x;
            down_y = y;
        }
        if (fabs(x - down_x) * canvas_width > 8 || fabs(y - down_y) * canvas_height > 8)
        {
            tap_eligible = 0;
            tap_time = 0;
        }
        message->code = 1;
        message->flags = gesture ? 2 : 1;
        message->first = (long)(x * canvas_width * 20);
        message->second = (long)(y * canvas_height * 20);
        if (!gesture && event->type == SDL_FINGERDOWN && tap_time && (Uint32)(event->timestamp - tap_time) <= 350 &&
            fabs(x * canvas_width - tap_x) <= 8 && fabs(y * canvas_height - tap_y) <= 8 &&
            (dvm_desktop_header(x * canvas_width, y * canvas_height) ||
             dvm_desktop_zoom_button(x * canvas_width, y * canvas_height)))
        {
            message->code = dvm_desktop_zoom_button(x * canvas_width, y * canvas_height) ? 12 : 10;
            tap_time = 0;
            tap_eligible = 0;
        }
        gesture = 1;
        span = 0;
        return 1;
    }
    if (!count || count > 3)
    {
        span = 0;
        tap_time = 0;
        return 0;
    }
    tap_eligible = 0;
    tap_time = 0; /* Multi-contact gestures cannot complete a single-contact double tap. */
    if (count == 2)
    {
        double distance = hypot(xs[first] - xs[second], ys[first] - ys[second]);
        unsigned int anchor = 8;
        if (dvm_desktop_header(xs[first] * canvas_width, ys[first] * canvas_height))
        {
            anchor = first;
        }
        else if (dvm_desktop_header(xs[second] * canvas_width, ys[second] * canvas_height))
        {
            anchor = second;
        }
        if (span && fabs(distance - span) >= EDITOR_PINCH_THRESHOLD)
        {
            message->code = distance > span ? 8 : 9;
            message->flags = 1;
            message->first = (long)((anchor < 8 ? xs[anchor] : x / count) * canvas_width * 20);
            message->second = (long)((anchor < 8 ? ys[anchor] : y / count) * canvas_height * 20);
            span = distance;
            gesture = 0;
            zoom_pending = 1;
            return 1;
        }
        if (!span)
        {
            span = distance;
        }
    }
    if (count == 3 && !gesture)
    {
        triple_x = x / count;
        triple_y = y / count;
    }
    message->code = count == 3 ? 7 : 6;
    message->flags = gesture ? 2 : 1;
    message->first = (long)((count == 3 ? first_x + x / count - triple_x : x / count) * canvas_width * 20);
    message->second = (long)((count == 3 ? first_y + y / count - triple_y : y / count) * canvas_height * 20);
    gesture = count;
    return 1;
}
#endif


/* Parse a bounded canonical metadata word; text is eight borrowed ASCII bytes.
 * Returns one with out set, or zero without publishing an invalid value. */
static int hexword(const char *text, unsigned long *out)
{
    unsigned int index;
    unsigned long value = 0;
    for (index = 0; index < 8; index++)
    {
        unsigned int digit = (unsigned char)text[index];
        if (digit >= '0' && digit <= '9')
        {
            digit -= '0';
        }
        else if (digit >= 'a' && digit <= 'f')
        {
            digit = digit - 'a' + 10;
        }
        else
        {
            return 0;
        }
        value = (value << 4) | digit;
    }
    *out = value;
    return 1;
}

/* Return startup configuration by value; no renderer or document state is exposed. */
int dvm_editor_line_numbers(void)
{
    return initial_line_numbers;
}
