/* GTK3 owner-thread frontend: opaque RGBA drawing, scaled pointer and focus cancellation. */
#include "../common/wheel.h" /* Reuse public C89 VM/frame ownership and rendering. */
#include "limits.h"          /* Native channel precision used for GdkPixbuf. */
#include <gtk/gtk.h>         /* External GTK3/GDK/GObject/Cairo SDK. */
#include <stdio.h>           /* Print actual status and bounded smoke results. */

/* Native callback state never escapes the main thread. */
typedef struct GtkHost        /* The stack owner outlives gtk_main and every source. */
{                             /* The stack owner outlives gtk_main and every source. */
    Demo demo;                /* Independent runtime/context/pixel owners. */
    GtkWidget *area;          /* Borrowed child, owned by the window. */
    cairo_t *cr;              /* Borrowed only while handling draw. */
    int smoke;                /* Explicit finite rendering mode. */
    GdkCursor *hidden_cursor; /* Owned blank native descriptor, acquired only if a hidden request is applied. */
    int cursor_applied;       /* Last request issued to this drawing area's GdkWindow, or -1 before first apply. */
} GtkHost;                    /* No managed script pointers need GC pins. */


/* Apply requested cursor only to the realized drawing area, leaving the marker/content visible. */
static FVM_Result gtk_cursor(GtkHost *host) /* Native paint/event owner thread only. */
{                                           /* A request is independent of native device presence. */
    int visible;                            /* Query output is used only on success. */
    GdkWindow *window;                      /* Borrow the live realized widget window. */
    if (host->demo.status != FVM_OK)        /* Condition for this action: Keep the first error. */
    {                                       /* Begin conditional branch: Condition for this action: Keep the first error. */
        return host->demo.status;           /* Keep the first error. */
    } /* End conditional branch: Condition for this action: Keep the first error. */
    host->demo.status = fvm_input_get_cursor_visible(host->demo.context, &visible); /* No GC or SDK inside VM. */
    if (host->demo.status != FVM_OK || visible == host->cursor_applied)             /* Condition for this action: No repeat. */
    {                                                                               /* Begin conditional branch: Condition for this action: No repeat. */
        return host->demo.status;                                                   /* No repeat. */
    } /* End conditional branch: Condition for this action: No repeat. */
    window = gtk_widget_get_window(host->area); /* Valid only after native realization. */
    if (!window)                                /* Condition for this action: Do not claim an unapplied native request. */
    {                                           /* Begin conditional branch: Condition for this action: Do not claim an unapplied native request. */
        return host->demo.status = FVM_ERROR;   /* Do not claim an unapplied native request. */
    } /* End conditional branch: Condition for this action: Do not claim an unapplied native request. */
    if (!visible && !host->hidden_cursor)                                                                       /* Condition for this action: Lazily acquire only the required native resource. */
    {                                                                                                           /* Lazily acquire only the required native resource. */
        host->hidden_cursor = gdk_cursor_new_for_display(gtk_widget_get_display(host->area), GDK_BLANK_CURSOR); /* Owned. */
        if (!host->hidden_cursor)                                                                               /* Condition for this action: SDK acquisition failure. */
        {                                                                                                       /* Begin conditional branch: Condition for this action: SDK acquisition failure. */
            return host->demo.status = FVM_ERROR;                                                               /* SDK acquisition failure. */
        } /* End conditional branch: Condition for this action: SDK acquisition failure. */
    } /* GDK retains its own cursor reference when applying below. */
    gdk_window_set_cursor(window, visible ? NULL : host->hidden_cursor); /* NULL restores inherited cursor. */
    host->cursor_applied = visible;                                      /* Void SDK API records issuance, not a compositor/device acknowledgement. */
    return FVM_OK;                                                       /* Host keeps its blank descriptor until native window teardown. */
} /* No global display cursor or other window's request is modified. */


/* Consume the frame before its producer may overwrite the bytes. */
static FVM_Result gtk_present(const FVM_RasterSurface *frame, void *data)                                                                      /* Synchronous sink. */
{                                                                                                                                              /* The demo is opaque, so straight/premultiplied alpha coincide. */
    GtkHost *host = (GtkHost *)data;                                                                                                           /* Borrow the live main-thread host. */
    GdkPixbuf *image = gdk_pixbuf_new_from_data(frame->pixels, GDK_COLORSPACE_RGB, TRUE,                                                       /* Borrow RGBA. */
                                                GTK_DEMO_CHANNEL_BITS, (int)frame->width, (int)frame->height, (int)frame->stride, NULL, NULL); /* No destroy callback. */
    if (!image)                                                                                                                                /* Condition for this action: No retained pixel allocation on failure. */
    {                                                                                                                                          /* Begin conditional branch: Condition for this action: No retained pixel allocation on failure. */
        return FVM_OUT_OF_MEMORY;                                                                                                              /* No retained pixel allocation on failure. */
    } /* End conditional branch: Condition for this action: No retained pixel allocation on failure. */
    cairo_save(host->cr);                                                                    /* Preserve GTK's clip and transform. */
    cairo_scale(host->cr, (double)gtk_widget_get_allocated_width(host->area) / frame->width, /* Logical X. */
                (double)gtk_widget_get_allocated_height(host->area) / frame->height);        /* Same scale as input inverse. */
    gdk_cairo_set_source_pixbuf(host->cr, image, 0, 0);                                      /* Cairo borrows a reference through painting. */
    cairo_paint(host->cr);                                                                   /* Paint the opaque frame inside GTK's existing clip. */
    cairo_restore(host->cr);                                                                 /* Release the source before producer reuse. */
    g_object_unref(image);                                                                   /* Release descriptor; pixel storage remains owned by Demo. */
    return cairo_status(host->cr) == CAIRO_STATUS_SUCCESS ? FVM_OK : FVM_ERROR;              /* Real sink status. */
} /* No source pointer survives this synchronous call. */


/* Draw only in the GTK paint callback, where the Cairo context is valid. */
static gboolean gtk_draw(GtkWidget *widget, cairo_t *cr, gpointer data)                      /* Borrow paint parameters. */
{                                                                                            /* Repaint requests may be coalesced by GTK. */
    GtkHost *host = (GtkHost *)data;                                                         /* Lifetime is bounded by the native loop. */
    (void)widget;                                                                            /* The stored drawing area is the same signal sender. */
    gtk_cursor(host);                                                                        /* Apply current script/host request on the native paint owner. */
    host->cr = cr;                                                                           /* Borrow only until this callback returns. */
    demo_frame(&host->demo, gtk_present, host);                                              /* Draw and synchronously upload through public ABI. */
    host->cr = NULL;                                                                         /* Make the temporary borrowing boundary explicit. */
    if (host->demo.status != FVM_OK || (host->smoke && host->demo.frames >= DEMO_MAX_STEPS)) /* End finite run. */
    {                                                                                        /* Begin conditional branch: End finite run. */
        if (gtk_main_level())                                                                /* Condition for this action: Main stops the timer before destroying the VM. */
        {                                                                                    /* Begin conditional branch: Condition for this action: Main stops the timer before destroying the VM. */
            gtk_main_quit();                                                                 /* Main stops the timer before destroying the VM. */
        } /* End conditional branch: Condition for this action: Main stops the timer before destroying the VM. */
    } /* End conditional branch: End finite run. */
    return TRUE; /* This handler has painted the drawing area. */
} /* GTK retains ownership of its Cairo context. */


/* Convert widget logical coordinates back into the fixed world once. */
static gboolean gtk_pointer(GtkWidget *widget, GdkEvent *event, gpointer data)    /* Unified pointer signal. */
{                                                                                 /* Only primary mouse transitions are demonstrated here. */
    GtkHost *host = (GtkHost *)data;                                              /* Main owns the callback target. */
    FVM_Affine transform = {1, 0, 0, 1, 0, 0};                                    /* World to widget logical pixels. */
    FVM_MouseEventType type;                                                      /* Set only for the handled GDK variants. */
    double x, y;                                                                  /* Position in the sender widget, not device pixels. */
    transform.xx = (double)gtk_widget_get_allocated_width(widget) / DEMO_WIDTH;   /* Forward X scale. */
    transform.yy = (double)gtk_widget_get_allocated_height(widget) / DEMO_HEIGHT; /* Forward Y scale. */
    if (event->type == GDK_MOTION_NOTIFY)                                         /* Condition for this action: Motion requires no button to update the pointer. */
    {                                                                             /* Motion requires no button to update the pointer. */
        type = FVM_MOUSE_EVENT_MOVE;                                              /* Copy event data. */
        x = event->motion.x;                                                      /* Copy event data. */
        y = event->motion.y;                                                      /* Copy event data. */
    } /* End conditional branch: Condition for this action: Motion requires no button to update the pointer. */
    else if ((event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) &&      /* Button events. */
             event->button.button == DEMO_PRIMARY_BUTTON)                                   /* Ignore other buttons/double-click signals. */
    {                                                                                       /* Ignore other buttons/double-click signals. */
        type = event->type == GDK_BUTTON_PRESS ? FVM_MOUSE_EVENT_DOWN : FVM_MOUSE_EVENT_UP; /* Transition. */
        x = event->button.x;                                                                /* Local logical pixels. */
        y = event->button.y;                                                                /* Local logical pixels. */
    } /* End scope: Ignore other buttons/double-click signals. */
    else              /* Unhandled signals remain available to GTK. */
    {                 /* Begin conditional branch: Unhandled signals remain available to GTK. */
        return FALSE; /* Unhandled signals remain available to GTK. */
    } /* End conditional branch: Unhandled signals remain available to GTK. */
    demo_pointer(&host->demo, type, &transform, x, y); /* Owner-thread VM entry and status latch. */
    if (host->demo.status != FVM_OK)                   /* Condition for this action: Surface mapping/VM errors to main. */
    {                                                  /* Begin conditional branch: Condition for this action: Surface mapping/VM errors to main. */
        gtk_main_quit();                               /* Surface mapping/VM errors to main. */
    } /* End conditional branch: Condition for this action: Surface mapping/VM errors to main. */
    return TRUE; /* This pointer signal was consumed. */
} /* No keyboard, IME or touch recognizer is implied. */


/* Convert GDK vertical wheel/smooth units with an explicit three-line tutorial policy. */
static gboolean gtk_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer data) /* Borrow event. */
{                                                                                   /* GDK down-positive smooth deltas are inverted into the VM up-positive line convention. */
    GtkHost *host = (GtkHost *)data;                                                /* Main owns the callback target. */
    FVM_Affine transform = {1, 0, 0, 1, 0, 0};                                      /* Same logical scale as Cairo painting. */
    double delta, dx, dy;                                                           /* Native smooth outputs are valid only when the query succeeds. */
    if (event->direction == GDK_SCROLL_UP)                                          /* Condition for this action: Discrete upward step. */
    {                                                                               /* Begin conditional branch: Condition for this action: Discrete upward step. */
        delta = DEMO_WHEEL_LINES_PER_DETENT;                                        /* Discrete upward step. */
    } /* End conditional branch: Condition for this action: Discrete upward step. */
    else if (event->direction == GDK_SCROLL_DOWN) /* Discrete down. */
    {                                             /* Begin conditional branch: Discrete down. */
        delta = -DEMO_WHEEL_LINES_PER_DETENT;     /* Discrete down. */
    } /* End conditional branch: Discrete down. */
    else if (event->direction == GDK_SCROLL_SMOOTH &&                                   /* Stop/zero/horizontal-only input is not a vertical event. */
             gdk_event_get_scroll_deltas((const GdkEvent *)event, &dx, &dy) && dy != 0) /* Borrow scalar deltas. */
    {                                                                                   /* Begin scope: Borrow scalar deltas. */
        delta = -dy * DEMO_WHEEL_LINES_PER_DETENT;                                      /* Explicit smooth-unit policy, not pixel-to-line inference. */
    } /* End scope: Borrow scalar deltas. */
    else              /* Let GTK handle horizontal or unsupported scroll protocols. */
    {                 /* Begin conditional branch: Let GTK handle horizontal or unsupported scroll protocols. */
        return FALSE; /* Let GTK handle horizontal or unsupported scroll protocols. */
    } /* End conditional branch: Let GTK handle horizontal or unsupported scroll protocols. */
    transform.xx = (double)gtk_widget_get_allocated_width(widget) / DEMO_WIDTH;   /* Forward widget X scale. */
    transform.yy = (double)gtk_widget_get_allocated_height(widget) / DEMO_HEIGHT; /* Forward widget Y scale. */
    demo_wheel(&host->demo, &transform, event->x, event->y, delta);               /* Fractional event position maps once. */
    if (host->demo.status != FVM_OK && gtk_main_level())                          /* Condition for this action: Stop a live native loop on error. */
    {                                                                             /* Begin conditional branch: Condition for this action: Stop a live native loop on error. */
        gtk_main_quit();                                                          /* Stop a live native loop on error. */
    } /* End conditional branch: Condition for this action: Stop a live native loop on error. */
    return TRUE; /* A recognized vertical wheel event was consumed. */
} /* No click, focus change or scrollTarget is synthesized. */


/* Cancel an active pointer when the native window loses focus. */
static gboolean gtk_focus(GtkWidget *widget, GdkEventFocus *event, gpointer data) /* Focus-out signal. */
{                                                                                 /* The common helper releases at the last valid VM coordinate. */
    GtkHost *host = (GtkHost *)data;                                              /* Borrow callback owner. */
    (void)widget;                                                                 /* Cancel does not require fresh pointer coordinates. */
    (void)event;                                                                  /* Cancel does not require fresh pointer coordinates. */
    demo_cancel(&host->demo);                                                     /* Explicit synthetic-UP policy. */
    return FALSE;                                                                 /* Let GTK continue its own focus processing. */
} /* The VM does not automatically change script Selection. */


/* Request a repaint; never store a Cairo context across ticks. */
static gboolean gtk_tick(gpointer data) /* GLib timer on the same main context. */
{                                       /* Sources are removed before the stack host is released. */
    GtkHost *host = (GtkHost *)data;    /* Live callback target. */
    gtk_widget_queue_draw(host->area);  /* Native loop schedules paint when drawable. */
    return G_SOURCE_CONTINUE;           /* Main explicitly removes this source at shutdown. */
} /* Timer pacing is not a movie/audio clock. */


/* Native close ends the main loop without destroying Demo inside the signal. */
static void gtk_closed(GtkWidget *widget, gpointer data) /* Window destroy signal. */
{                                                        /* Main retains a window reference until cleanup. */
    (void)widget;                                        /* No callback owner is released here. */
    (void)data;                                          /* No callback owner is released here. */
    if (gtk_main_level())                                /* Condition for this action: Cleanup may also destroy after the native loop already ended. */
    {                                                    /* Begin conditional branch: Condition for this action: Cleanup may also destroy after the native loop already ended. */
        gtk_main_quit();                                 /* Cleanup may also destroy after the native loop already ended. */
    } /* End conditional branch: Condition for this action: Cleanup may also destroy after the native loop already ended. */
} /* No timer can run after main removes it. */


/* Run interactively or with --smoke for 120 successful draw callbacks. */
int main(int argc, char **argv)                      /* GTK owns all native dispatch on this thread. */
{                                                    /* Validate options before GTK can consume its own arguments. */
    GtkHost host;                                    /* Zeroed before partial resource acquisition. */
    GtkWidget *window;                               /* Explicitly referenced native owner. */
    guint timer;                                     /* Source ID removed after the loop. */
    int result, argument, hide_cursor = 0, previous; /* Native options and mandatory previous-request output. */
    memset(&host, 0, sizeof(host));                  /* Initialize callbacks and VM ownership. */
    host.cursor_applied = -1;                        /* Native inherited cursor has not yet been synchronized with the context. */
    for (argument = 1; argument < argc; ++argument)  /* Parse flags before initializing the SDK. */
    {                                                /* Parse flags before initializing the SDK. */
        if (!strcmp(argv[argument], "--smoke"))      /* Condition for this action: Bound successful draw callbacks. */
        {                                            /* Begin conditional branch: Condition for this action: Bound successful draw callbacks. */
            host.smoke = 1;                          /* Bound successful draw callbacks. */
        } /* End conditional branch: Condition for this action: Bound successful draw callbacks. */
        else if (!strcmp(argv[argument], "--hide-cursor")) /* Independent VM request. */
        {                                                  /* Begin conditional branch: Independent VM request. */
            hide_cursor = 1;                               /* Independent VM request. */
        } /* End conditional branch: Independent VM request. */
        else                                                                   /* No native owners. */
        {                                                                      /* Begin conditional branch: No native owners. */
            fprintf(stderr, "usage: %s [--smoke] [--hide-cursor]\n", argv[0]); /* No native owners. */
            return 1;                                                          /* No native owners. */
        } /* No native owners. */
    } /* A hidden-cursor software run can combine both options. */
    if (!gtk_init_check(NULL, NULL))                  /* Condition for this action: Real SDK. */
    {                                                 /* Begin conditional branch: Condition for this action: Real SDK. */
        fprintf(stderr, "GTK display unavailable\n"); /* Real SDK. */
        return 1;                                     /* Real SDK. */
    } /* Real SDK. */
    if (demo_open(&host.demo) != FVM_OK) /* Condition for this action: Partial cleanup. */
    {                                    /* Begin conditional branch: Condition for this action: Partial cleanup. */
        demo_close(&host.demo);          /* Partial cleanup. */
        return 1;                        /* Partial cleanup. */
    } /* Partial cleanup. */
    if (hide_cursor && fvm_input_set_cursor_visible(host.demo.context, 0, &previous) != FVM_OK) /* Condition for this action: Startup request. */
    {                                                                                           /* Startup request. */
        demo_close(&host.demo);                                                                 /* No native window has been acquired yet. */
        return 1;                                                                               /* No native window has been acquired yet. */
    } /* The next draw callback applies this request after realization. */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);                                                                                                                /* Own a real native top-level window. */
    g_object_ref_sink(window);                                                                                                                                   /* Keep the object valid even after a destroy signal. */
    host.area = gtk_drawing_area_new();                                                                                                                          /* Window owns its child once added. */
    gtk_widget_set_size_request(host.area, DEMO_WIDTH, DEMO_HEIGHT);                                                                                             /* Minimum logical extent. */
    gtk_widget_add_events(host.area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |                                                                           /* Enable input. */
                                         GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);                                                    /* Listen to releases as well as presses. */
    gtk_container_add(GTK_CONTAINER(window), host.area);                                                                                                         /* Transfer child ownership to window. */
    g_signal_connect(host.area, "draw", G_CALLBACK(gtk_draw), &host);                                                                                            /* Paint context lifetime. */
    g_signal_connect(host.area, "motion-notify-event", G_CALLBACK(gtk_pointer), &host);                                                                          /* Hover/drag. */
    g_signal_connect(host.area, "button-press-event", G_CALLBACK(gtk_pointer), &host);                                                                           /* Primary down. */
    g_signal_connect(host.area, "button-release-event", G_CALLBACK(gtk_pointer), &host);                                                                         /* Primary up. */
    g_signal_connect(host.area, "scroll-event", G_CALLBACK(gtk_scroll), &host);                                                                                  /* Vertical wheel/smooth units. */
    g_signal_connect(window, "focus-out-event", G_CALLBACK(gtk_focus), &host);                                                                                   /* Cancel stale hold. */
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_closed), NULL);                                                                                           /* Stop native dispatch. */
    timer = g_timeout_add(DEMO_FRAME_MS, gtk_tick, &host);                                                                                                       /* Coarse owner-thread pacing. */
    gtk_widget_show_all(window);                                                                                                                                 /* Realize native resources before processing callbacks. */
    gtk_main();                                                                                                                                                  /* Run until native close, smoke completion or VM failure. */
    g_source_remove(timer);                                                                                                                                      /* No callback may access host after cleanup starts. */
    result = host.demo.status == FVM_OK ? 0 : 1;                                                                                                                 /* Capture status before demo_close resets it. */
    printf("GTK3 frames=%u wheels=%u last_delta=%.3f status=%d\n", host.demo.frames, host.demo.wheel_events, host.demo.last_wheel_delta, (int)host.demo.status); /* Observable result. */
    gtk_widget_destroy(window);                                                                                                                                  /* Remove native signal senders before destroying VM. */
    g_object_unref(window);                                                                                                                                      /* Release the explicit native owner. */
    if (host.hidden_cursor)                                                                                                                                      /* Condition for this action: Release owned descriptor after window teardown. */
    {                                                                                                                                                            /* Begin conditional branch: Condition for this action: Release owned descriptor after window teardown. */
        g_object_unref(host.hidden_cursor);                                                                                                                      /* Release owned descriptor after window teardown. */
    } /* End conditional branch: Condition for this action: Release owned descriptor after window teardown. */
    demo_close(&host.demo); /* [GC:] Release context/runtime after native callbacks stop. */
    return result;          /* Initialization/backend failures remain nonzero. */
} /* This is an embedding frontend, not a complete SWF player. */
