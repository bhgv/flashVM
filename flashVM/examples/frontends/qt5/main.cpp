/* Qt5 frontend: SDK-native widget and QImage consumer around the public FlashVM C ABI. */
#include "../common/wheel.h" /* Common VM/frame lifecycle; core remains C++98/C89. */
#include <QCursor>           /* Native widget-local blank/inherited cursor, independent of frame content. */
#include <QApplication>      /* Native GUI main-thread event loop. */
#include <QWidget>           /* Native owner with virtual event callbacks, no moc required. */
#include <QPainter>          /* Paint only within paintEvent. */
#include <QImage>            /* Byte-ordered RGBA8888 view over borrowed native bytes. */
#include "limits.h"          /* Qt angle-unit normalization, separate from generic VM scroll lines. */
#include <QWheelEvent>       /* Native angle deltas and fractional widget event position. */
#include <QMouseEvent>       /* Logical widget pointer coordinates. */
#include <QFocusEvent>       /* Explicit focus-loss cancellation. */
#include <QTimerEvent>       /* Timer ownership without signal/slot glue. */
#include <cstdio>            /* Report successful frames and real VM status. */

/* All VM and native objects remain on QApplication's GUI thread. */
class DemoWidget : public QWidget /* Qt SDK host; no managed object escapes. */
{                                 /* Qt SDK host; no managed object escapes. */
    Demo demo_;                   /* Own VM, context and pixel allocation until widget teardown. */
    QPainter *painter_;           /* Borrowed only during paintEvent. */
    int timer_;                   /* Native timer ID, stopped before VM release. */
    bool smoke_;                  /* Stop after a bounded number of successful paints. */
    int cursor_applied_;          /* Last request issued to this widget, or -1 before synchronization. */
    /* Copying a live VM/native owner is forbidden. */
    DemoWidget(const DemoWidget &);            /* No shallow ownership copies. */
    DemoWidget &operator=(const DemoWidget &); /* No owner reassignment. */


    /* Present one opaque frame without retaining any producer bytes. */
    static FVM_Result present(const FVM_RasterSurface *frame, void *data) /* Synchronous ABI sink. */
    {                                                                     /* Synchronous ABI sink. */


        DemoWidget *host = static_cast<DemoWidget *>(data);                /* Owner outlives the paint callback. */
        QImage image(frame->pixels, (int)frame->width, (int)frame->height, /* Borrow native RGBA. */
                     (int)frame->stride, QImage::Format_RGBA8888);         /* Byte order independent of CPU endian. */
        if (image.isNull())                                                /* Condition for this action: Report an unusable native image. */
        {                                                                  /* Begin conditional branch: Condition for this action: Report an unusable native image. */
            return FVM_ERROR;                                              /* Report an unusable native image. */
        } /* End conditional branch: Condition for this action: Report an unusable native image. */
        host->painter_->drawImage(host->rect(), image); /* Scale to widget logical coordinates. */
        return FVM_OK;                                  /* Image and painter release their borrowed span before return. */
    } /* QImage is never stored across frames. */


    /* Invert the same logical scale used by drawImage exactly once. */
    void pointer(QMouseEvent *event, FVM_MouseEventType type)                                 /* Borrow callback event. */
    {                                                                                         /* Borrow callback event. */
        FVM_Affine transform = {1, 0, 0, 1, 0, 0};                                            /* World to logical widget pixels. */
        transform.xx = (double)width() / DEMO_WIDTH;                                          /* Native DPR is handled by Qt, not repeated here. */
        transform.yy = (double)height() / DEMO_HEIGHT;                                        /* Match nonuniform image resize. */
        demo_pointer(&demo_, type, &transform, event->localPos().x(), event->localPos().y()); /* Preserve fractions. */
        if (demo_.status != FVM_OK)                                                           /* Condition for this action: Leave cleanup until native callback returns. */
        {                                                                                     /* Begin conditional branch: Condition for this action: Leave cleanup until native callback returns. */
            close();                                                                          /* Leave cleanup until native callback returns. */
        } /* End conditional branch: Condition for this action: Leave cleanup until native callback returns. */
    } /* No implicit keyboard/touch/IME translation. */

  protected: /* Qt invokes these methods only on the widget owner thread. */


    /* Rasterize and consume within the native paint scope. */
    void paintEvent(QPaintEvent *event)                                           /* Qt owns event lifetime. */
    {                                                                             /* Qt owns event lifetime. */
        (void)event;                                                              /* Repaint the complete fixed logical image. */
        int visible;                                                              /* Allocation-free VM request query before native painting. */
        if (demo_.status == FVM_OK)                                               /* Condition for this action: Owner only. */
        {                                                                         /* Begin conditional branch: Condition for this action: Owner only. */
            demo_.status = fvm_input_get_cursor_visible(demo_.context, &visible); /* Owner only. */
        } /* End conditional branch: Condition for this action: Owner only. */
        if (demo_.status != FVM_OK) /* Condition for this action: Stop before using an invalid query result. */
        {                           /* Begin conditional branch: Condition for this action: Stop before using an invalid query result. */
            close();                /* Stop before using an invalid query result. */
            return;                 /* Stop before using an invalid query result. */
        } /* Stop before using an invalid query result. */
        if (visible != cursor_applied_) /* Condition for this action: Change only this widget's native cursor override. */
        {                               /* Change only this widget's native cursor override. */
            if (visible)                /* Condition for this action: Restore inherited cursor instead of replacing another owner's style. */
            {                           /* Begin conditional branch: Condition for this action: Restore inherited cursor instead of replacing another owner's style. */
                unsetCursor();          /* Restore inherited cursor instead of replacing another owner's style. */
            } /* End conditional branch: Condition for this action: Restore inherited cursor instead of replacing another owner's style. */
            else                                     /* Qt owns the copied native descriptor. */
            {                                        /* Begin conditional branch: Qt owns the copied native descriptor. */
                setCursor(QCursor(Qt::BlankCursor)); /* Qt owns the copied native descriptor. */
            } /* End conditional branch: Qt owns the copied native descriptor. */
            cursor_applied_ = visible; /* Void SDK calls acknowledge issuance only, not physical scanout. */


        } /* The raster marker remains independent content, useful as a custom cursor. */
        QPainter painter(this);       /* Stack owner ends before this callback returns. */
        if (!painter.isActive())      /* Condition for this action: Real paint failure. */
        {                             /* Begin conditional branch: Condition for this action: Real paint failure. */
            demo_.status = FVM_ERROR; /* Real paint failure. */
            close();                  /* Real paint failure. */
            return;                   /* Real paint failure. */
        } /* Real paint failure. */
        painter_ = &painter;                                                      /* Temporarily expose sink for demo_frame. */
        demo_frame(&demo_, present, this);                                        /* Read VM pointer, render, present synchronously. */
        painter_ = 0;                                                             /* No dangling painter escapes this scope. */
        if (demo_.status != FVM_OK || (smoke_ && demo_.frames >= DEMO_MAX_STEPS)) /* Condition for this action: Finite smoke. */
        {                                                                         /* Begin conditional branch: Condition for this action: Finite smoke. */
            close();                                                              /* Finite smoke. */
        } /* End conditional branch: Condition for this action: Finite smoke. */
    } /* Stack painter releases native resources before the next event. */


    /* Schedule painting without entering VM from a second thread. */
    void timerEvent(QTimerEvent *event) /* Ignore unrelated native timers. */
    {                                   /* Ignore unrelated native timers. */
        if (event->timerId() == timer_) /* Condition for this action: Native repaint coalescing is permitted. */
        {                               /* Begin conditional branch: Condition for this action: Native repaint coalescing is permitted. */
            update();                   /* Native repaint coalescing is permitted. */
        } /* End conditional branch: Condition for this action: Native repaint coalescing is permitted. */
    } /* Timer is not an audio or movie clock. */


    /* Forward hover and drag position. */
    void mouseMoveEvent(QMouseEvent *event)   /* Logical coordinates. */
    {                                         /* Begin scope: Logical coordinates. */
        pointer(event, FVM_MOUSE_EVENT_MOVE); /* Logical coordinates. */
    } /* Logical coordinates. */


    /* Forward only the primary button press. */
    void mousePressEvent(QMouseEvent *event)      /* Other buttons keep their native meaning. */
    {                                             /* Other buttons keep their native meaning. */
        if (event->button() == Qt::LeftButton)    /* Condition for this action: One pointer stream. */
        {                                         /* Begin conditional branch: Condition for this action: One pointer stream. */
            pointer(event, FVM_MOUSE_EVENT_DOWN); /* One pointer stream. */
        } /* End conditional branch: Condition for this action: One pointer stream. */
    } /* No synthetic secondary contact promotion. */


    /* Forward only the matching primary button category. */
    void mouseReleaseEvent(QMouseEvent *event)  /* The common owner tracks held state. */
    {                                           /* The common owner tracks held state. */
        if (event->button() == Qt::LeftButton)  /* Condition for this action: Native release. */
        {                                       /* Begin conditional branch: Condition for this action: Native release. */
            pointer(event, FVM_MOUSE_EVENT_UP); /* Native release. */
        } /* End conditional branch: Condition for this action: Native release. */
    } /* Input status is latched by common/demo.h. */


    /* Convert vertical angle units into explicit lines; do not relabel pixelDelta as line units. */
    void wheelEvent(QWheelEvent *event) /* Qt owns this event until the native callback returns. */
    {                                   /* Qt owns this event until the native callback returns. */
        if (!event->angleDelta().y())   /* Condition for this action: Horizontal/pixel-only input remains native. */
        {                               /* Begin conditional branch: Condition for this action: Horizontal/pixel-only input remains native. */
            event->ignore();            /* Horizontal/pixel-only input remains native. */
            return;                     /* Horizontal/pixel-only input remains native. */
        } /* Horizontal/pixel-only input remains native. */
        FVM_Affine transform = {1, 0, 0, 1, 0, 0};                                                       /* Match drawImage's logical coordinate transform. */
        transform.xx = (double)width() / DEMO_WIDTH;                                                     /* DPR is already handled by Qt. */
        transform.yy = (double)height() / DEMO_HEIGHT;                                                   /* Nonuniform resize remains consistent with input. */
        double delta = event->angleDelta().y() / QT_DEMO_ANGLE_PER_DETENT * DEMO_WHEEL_LINES_PER_DETENT; /* Lines. */
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)                                                             /* Prefer the current API without dropping earlier Qt5 headers. */
        QPointF event_position = event->position();                                                      /* Borrow/copy event-time logical widget coordinates. */
#else                                                                                                    /* Qt5 5.5--5.13 expose the equivalent fractional legacy accessor. */
        QPointF event_position = event->posF(); /* Avoid integer rounding on supported older SDKs. */
#endif                                                                                                   /* Position is converted to world exactly once below. */
        demo_wheel(&demo_, &transform, event_position.x(), event_position.y(), delta);                   /* Preserve fraction/sign. */
        event->accept();                                                                                 /* Consume only a recognized vertical angle stream. */
        if (demo_.status != FVM_OK)                                                                      /* Condition for this action: End native delivery before VM cleanup. */
        {                                                                                                /* Begin conditional branch: Condition for this action: End native delivery before VM cleanup. */
            close();                                                                                     /* End native delivery before VM cleanup. */
        } /* End conditional branch: Condition for this action: End native delivery before VM cleanup. */
    } /* Qt's delivered sign is honored without applying its natural-scroll inversion a second time. */


    /* Release a stale hold on window deactivation, even if widget focus never changed. */
    bool event(QEvent *event)                          /* Override the generic native lifecycle dispatcher. */
    {                                                  /* Override the generic native lifecycle dispatcher. */
        if (event->type() == QEvent::WindowDeactivate) /* Condition for this action: Explicit synthetic UP. */
        {                                              /* Begin conditional branch: Condition for this action: Explicit synthetic UP. */
            demo_cancel(&demo_);                       /* Explicit synthetic UP. */
        } /* End conditional branch: Condition for this action: Explicit synthetic UP. */
        return QWidget::event(event); /* Preserve Qt's remaining native processing. */
    } /* No managed owner is destroyed from event callbacks. */

  public: /* Main owns this stack widget until after QApplication::exec returns. */


    /* Acquire a portable VM before requesting any paints. */
    explicit DemoWidget(bool smoke)                                  /* Zero native guards. */
        : painter_(0), timer_(0), smoke_(smoke), cursor_applied_(-1) /* Zero native guards. */
    {                                                                /* Zero native guards. */
        demo_open(&demo_);                                           /* Common helper initializes partial ownership before allocation. */
        resize(DEMO_WIDTH, DEMO_HEIGHT);                             /* Fixed world image, scalable native widget. */
        setMouseTracking(true);                                      /* Receive motion without a pressed button. */
        if (demo_.status == FVM_OK)                                  /* Condition for this action: Owner-thread pacing only. */
        {                                                            /* Begin conditional branch: Condition for this action: Owner-thread pacing only. */
            timer_ = startTimer(DEMO_FRAME_MS);                      /* Owner-thread pacing only. */
        } /* End conditional branch: Condition for this action: Owner-thread pacing only. */
        if (!timer_ && demo_.status == FVM_OK) /* Condition for this action: Native acquisition failure. */
        {                                      /* Begin conditional branch: Condition for this action: Native acquisition failure. */
            demo_.status = FVM_ERROR;          /* Native acquisition failure. */
        } /* End conditional branch: Condition for this action: Native acquisition failure. */
    } /* Main inspects status before showing an unsuccessful owner. */

    /* Stop native callbacks before releasing VM and borrowed pixel storage. */
    ~DemoWidget()              /* No application event loop is active during stack teardown. */
    {                          /* No application event loop is active during stack teardown. */
        if (timer_)            /* Condition for this action: Explicitly stop delivery before demo_close. */
        {                      /* Begin conditional branch: Condition for this action: Explicitly stop delivery before demo_close. */
            killTimer(timer_); /* Explicitly stop delivery before demo_close. */
        } /* End conditional branch: Condition for this action: Explicitly stop delivery before demo_close. */
        demo_close(&demo_); /* [GC:] No managed pointer escaped the context. */
    } /* QWidget base then releases its native resources. */


    /* Configure the context request from native UI/startup, then let the paint owner apply it. */
    FVM_Result request_cursor(int visible) /* This convenience method does not directly call a cursor SDK. */
    {                                      /* This convenience method does not directly call a cursor SDK. */
        int previous;                      /* Mandatory previous-request output is local native storage. */
        if (demo_.status != FVM_OK)        /* Condition for this action: Preserve a prior failure. */
        {                                  /* Begin conditional branch: Condition for this action: Preserve a prior failure. */
            return demo_.status;           /* Preserve a prior failure. */
        } /* End conditional branch: Condition for this action: Preserve a prior failure. */
        demo_.status = fvm_input_set_cursor_visible(demo_.context, visible, &previous); /* Validate 0/1 atomically. */
        if (demo_.status == FVM_OK)                                                     /* Condition for this action: Repaint will synchronize the widget's native cursor. */
        {                                                                               /* Begin conditional branch: Condition for this action: Repaint will synchronize the widget's native cursor. */
            update();                                                                   /* Repaint will synchronize the widget's native cursor. */
        } /* End conditional branch: Condition for this action: Repaint will synchronize the widget's native cursor. */
        return demo_.status; /* Caller observes real VM validation status. */
    } /* Method stays on the GUI/VM owner thread. */


    /* Return the first real failure without modifying ownership. */
    FVM_Result status() const /* Main can report before destructor clears state. */
    {                         /* Begin scope: Main can report before destructor clears state. */
        return demo_.status;  /* Main can report before destructor clears state. */
    } /* Main can report before destructor clears state. */


    /* Expose successful consumer count for a smoke-run assertion outside the widget. */
    unsigned int frames() const /* Not a physical scanout count. */
    {                           /* Begin scope: Not a physical scanout count. */
        return demo_.frames;    /* Not a physical scanout count. */
    } /* Not a physical scanout count. */


    /* Report successful wheel deliveries for host diagnostics and adapter fixtures. */
    unsigned int wheel_events() const /* Native scalar snapshot only. */
    {                                 /* Begin scope: Native scalar snapshot only. */
        return demo_.wheel_events;    /* Native scalar snapshot only. */
    } /* Native scalar snapshot only. */


    /* Report the last normalized line value without retaining a native event. */
    double last_wheel_delta() const    /* Zero before first accepted wheel. */
    {                                  /* Begin scope: Zero before first accepted wheel. */
        return demo_.last_wheel_delta; /* Zero before first accepted wheel. */
    } /* Zero before first accepted wheel. */
}; /* Qt types remain private to this host executable. */


/* Run with --smoke or interactively, using the selected Qt platform plugin. */
int main(int argc, char **argv)                         /* QApplication must outlive every QWidget. */
{                                                       /* QApplication must outlive every QWidget. */
    bool smoke = false, hide_cursor = false;            /* Explicit frame and cursor-request options. */
    for (int argument = 1; argument < argc; ++argument) /* Qt SDK host alone uses C++11. */
    {                                                   /* Qt SDK host alone uses C++11. */
        if (!strcmp(argv[argument], "--smoke"))         /* Condition for this action: Finite owner loop. */
        {                                               /* Begin conditional branch: Condition for this action: Finite owner loop. */
            smoke = true;                               /* Finite owner loop. */
        } /* End conditional branch: Condition for this action: Finite owner loop. */
        else if (!strcmp(argv[argument], "--hide-cursor")) /* No synthetic mouse event. */
        {                                                  /* Begin conditional branch: No synthetic mouse event. */
            hide_cursor = true;                            /* No synthetic mouse event. */
        } /* End conditional branch: No synthetic mouse event. */
        else          /* Choose the native Qt plugin through QT_QPA_PLATFORM environment. */
        {             /* Begin conditional branch: Choose the native Qt plugin through QT_QPA_PLATFORM environment. */
            return 1; /* Choose the native Qt plugin through QT_QPA_PLATFORM environment. */
        } /* End conditional branch: Choose the native Qt plugin through QT_QPA_PLATFORM environment. */


    } /* Known flags can be combined before QApplication consumes native options. */


    QApplication app(argc, argv); /* Initialize real SDK/platform integration. */
    DemoWidget widget(smoke);     /* Stack teardown precedes application destruction. */
    if (hide_cursor)              /* Condition for this action: Initial request is applied by the first paint. */
    {                             /* Begin conditional branch: Condition for this action: Initial request is applied by the first paint. */
        widget.request_cursor(0); /* Initial request is applied by the first paint. */
    } /* End conditional branch: Condition for this action: Initial request is applied by the first paint. */
    if (widget.status() != FVM_OK) /* Condition for this action: Partial owners are still released by destructor. */
    {                              /* Begin conditional branch: Condition for this action: Partial owners are still released by destructor. */
        return 1;                  /* Partial owners are still released by destructor. */
    } /* End conditional branch: Condition for this action: Partial owners are still released by destructor. */
    widget.show();                                                                                                                                               /* Schedule a native paint only after valid initialization. */
    int result = app.exec();                                                                                                                                     /* Deliver all events on this same GUI/VM thread. */
    std::printf("Qt5 frames=%u wheels=%u last_delta=%.3f status=%d\n", widget.frames(), widget.wheel_events(), widget.last_wheel_delta(), (int)widget.status()); /* Observable smoke. */
    return result || widget.status() != FVM_OK ? 1 : 0;                                                                                                          /* Preserve native and VM failures. */
} /* Qt5 SDK may require C++11; no C++11 is added to the VM library. */
