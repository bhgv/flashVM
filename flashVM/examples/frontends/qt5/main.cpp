/* Qt5 frontend: SDK-native widget and QImage consumer around the public FlashVM C ABI. */
#include "../common/demo.h" /* Common VM/frame lifecycle; core remains C++98/C89. */
#include <QApplication> /* Native GUI main-thread event loop. */
#include <QWidget> /* Native owner with virtual event callbacks, no moc required. */
#include <QPainter> /* Paint only within paintEvent. */
#include <QImage> /* Byte-ordered RGBA8888 view over borrowed native bytes. */
#include <QMouseEvent> /* Logical widget pointer coordinates. */
#include <QFocusEvent> /* Explicit focus-loss cancellation. */
#include <QTimerEvent> /* Timer ownership without signal/slot glue. */
#include <cstdio> /* Report successful frames and real VM status. */
/* All VM and native objects remain on QApplication's GUI thread. */
class DemoWidget : public QWidget { /* Qt SDK host; no managed object escapes. */
    Demo demo_; /* Own VM, context and pixel allocation until widget teardown. */
    QPainter *painter_; /* Borrowed only during paintEvent. */
    int timer_; /* Native timer ID, stopped before VM release. */
    bool smoke_; /* Stop after a bounded number of successful paints. */
    /* Copying a live VM/native owner is forbidden. */
    DemoWidget(const DemoWidget &); /* No shallow ownership copies. */
    DemoWidget &operator=(const DemoWidget &); /* No owner reassignment. */
    /* Present one opaque frame without retaining any producer bytes. */
    static FVM_Result present(const FVM_RasterSurface *frame, void *data) { /* Synchronous ABI sink. */
        DemoWidget *host = static_cast<DemoWidget *>(data); /* Owner outlives the paint callback. */
        QImage image(frame->pixels, (int)frame->width, (int)frame->height, /* Borrow native RGBA. */
            (int)frame->stride, QImage::Format_RGBA8888); /* Byte order independent of CPU endian. */
        if (image.isNull()) return FVM_ERROR; /* Report an unusable native image. */
        host->painter_->drawImage(host->rect(), image); /* Scale to widget logical coordinates. */
        return FVM_OK; /* Image and painter release their borrowed span before return. */
    } /* QImage is never stored across frames. */
    /* Invert the same logical scale used by drawImage exactly once. */
    void pointer(QMouseEvent *event, FVM_MouseEventType type) { /* Borrow callback event. */
        FVM_Affine transform = {1, 0, 0, 1, 0, 0}; /* World to logical widget pixels. */
        transform.xx = (double)width() / DEMO_WIDTH; /* Native DPR is handled by Qt, not repeated here. */
        transform.yy = (double)height() / DEMO_HEIGHT; /* Match nonuniform image resize. */
        demo_pointer(&demo_, type, &transform, event->localPos().x(), event->localPos().y()); /* Preserve fractions. */
        if (demo_.status != FVM_OK) close(); /* Leave cleanup until native callback returns. */
    } /* No implicit keyboard/touch/IME translation. */
protected: /* Qt invokes these methods only on the widget owner thread. */
    /* Rasterize and consume within the native paint scope. */
    void paintEvent(QPaintEvent *event) { /* Qt owns event lifetime. */
        (void)event; /* Repaint the complete fixed logical image. */
        QPainter painter(this); /* Stack owner ends before this callback returns. */
        if (!painter.isActive()) { demo_.status = FVM_ERROR; close(); return; } /* Real paint failure. */
        painter_ = &painter; /* Temporarily expose sink for demo_frame. */
        demo_frame(&demo_, present, this); /* Read VM pointer, render, present synchronously. */
        painter_ = 0; /* No dangling painter escapes this scope. */
        if (demo_.status != FVM_OK || (smoke_ && demo_.frames >= DEMO_MAX_STEPS)) close(); /* Finite smoke. */
    } /* Stack painter releases native resources before the next event. */
    /* Schedule painting without entering VM from a second thread. */
    void timerEvent(QTimerEvent *event) { /* Ignore unrelated native timers. */
        if (event->timerId() == timer_) update(); /* Native repaint coalescing is permitted. */
    } /* Timer is not an audio or movie clock. */
    /* Forward hover and drag position. */
    void mouseMoveEvent(QMouseEvent *event) { pointer(event, FVM_MOUSE_EVENT_MOVE); } /* Logical coordinates. */
    /* Forward only the primary button press. */
    void mousePressEvent(QMouseEvent *event) { /* Other buttons keep their native meaning. */
        if (event->button() == Qt::LeftButton) pointer(event, FVM_MOUSE_EVENT_DOWN); /* One pointer stream. */
    } /* No synthetic secondary contact promotion. */
    /* Forward only the matching primary button category. */
    void mouseReleaseEvent(QMouseEvent *event) { /* The common owner tracks held state. */
        if (event->button() == Qt::LeftButton) pointer(event, FVM_MOUSE_EVENT_UP); /* Native release. */
    } /* Input status is latched by common/demo.h. */
    /* Release a stale hold on window deactivation, even if widget focus never changed. */
    bool event(QEvent *event) { /* Override the generic native lifecycle dispatcher. */
        if (event->type() == QEvent::WindowDeactivate) demo_cancel(&demo_); /* Explicit synthetic UP. */
        return QWidget::event(event); /* Preserve Qt's remaining native processing. */
    } /* No managed owner is destroyed from event callbacks. */
public: /* Main owns this stack widget until after QApplication::exec returns. */
    /* Acquire a portable VM before requesting any paints. */
    explicit DemoWidget(bool smoke) : painter_(0), timer_(0), smoke_(smoke) { /* Zero native guards. */
        demo_open(&demo_); /* Common helper initializes partial ownership before allocation. */
        resize(DEMO_WIDTH, DEMO_HEIGHT); /* Fixed world image, scalable native widget. */
        setMouseTracking(true); /* Receive motion without a pressed button. */
        if (demo_.status == FVM_OK) timer_ = startTimer(DEMO_FRAME_MS); /* Owner-thread pacing only. */
        if (!timer_ && demo_.status == FVM_OK) demo_.status = FVM_ERROR; /* Native acquisition failure. */
    } /* Main inspects status before showing an unsuccessful owner. */
    /* Stop native callbacks before releasing VM and borrowed pixel storage. */
    ~DemoWidget() { /* No application event loop is active during stack teardown. */
        if (timer_) killTimer(timer_); /* Explicitly stop delivery before demo_close. */
        demo_close(&demo_); /* [GC:] No managed pointer escaped the context. */
    } /* QWidget base then releases its native resources. */
    /* Return the first real failure without modifying ownership. */
    FVM_Result status() const { return demo_.status; } /* Main can report before destructor clears state. */
    /* Expose successful consumer count for a smoke-run assertion outside the widget. */
    unsigned int frames() const { return demo_.frames; } /* Not a physical scanout count. */
}; /* Qt types remain private to this host executable. */
/* Run with --smoke or interactively, using the selected Qt platform plugin. */
int main(int argc, char **argv) { /* QApplication must outlive every QWidget. */
    bool smoke = argc == 2 && !strcmp(argv[1], "--smoke"); /* Parse tutorial arguments explicitly. */
    if (argc > 1 && !smoke) return 1; /* Select Qt plugin through QT_QPA_PLATFORM environment. */
    QApplication app(argc, argv); /* Initialize real SDK/platform integration. */
    DemoWidget widget(smoke); /* Stack teardown precedes application destruction. */
    if (widget.status() != FVM_OK) return 1; /* Partial owners are still released by destructor. */
    widget.show(); /* Schedule a native paint only after valid initialization. */
    int result = app.exec(); /* Deliver all events on this same GUI/VM thread. */
    std::printf("Qt5 frames=%u status=%d\n", widget.frames(), (int)widget.status()); /* Observable smoke. */
    return result || widget.status() != FVM_OK ? 1 : 0; /* Preserve native and VM failures. */
} /* Qt5 SDK may require C++11; no C++11 is added to the VM library. */
