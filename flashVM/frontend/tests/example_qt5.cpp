/* Real Qt5 SDK paint/input fixture; offscreen is software acceptance only. */
#define main qt_tutorial_entry
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/qt5/main.cpp"

#undef main


static void marker(DemoWidget &widget, int r, int g)
{
    QImage frame = widget.grab().toImage();
    if (!(!frame.isNull()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    QRgb pixel = frame.pixel(480, 300);
    if (!(qRed(pixel) == r && qGreen(pixel) == g && qBlue(pixel) == DEMO_MARKER_BLUE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv)
{


    QApplication app(argc, argv);


    DemoWidget widget(false);
    if (!(widget.status() == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    widget.resize(640, 400);
    widget.show();
    app.processEvents();
    if (!(widget.cursor().shape() == Qt::ArrowCursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(widget.request_cursor(0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    widget.grab();
    if (!(widget.cursor().shape() == Qt::BlankCursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(widget.request_cursor(1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    widget.grab();
    if (!(widget.cursor().shape() == Qt::ArrowCursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    QMouseEvent move(QEvent::MouseMove, QPointF(480.5, 300.25), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &move);
    marker(widget, DEMO_RELEASED_RED, DEMO_RELEASED_GREEN);


    QMouseEvent down(QEvent::MouseButtonPress, QPointF(480.5, 300.25), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &down);
    marker(widget, DEMO_HELD_RED, DEMO_HELD_GREEN);


    QEvent deactivate(QEvent::WindowDeactivate);
    QApplication::sendEvent(&widget, &deactivate);
    marker(widget, DEMO_RELEASED_RED, DEMO_RELEASED_GREEN);
    if (!(widget.status() == FVM_OK && widget.frames() >= 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)


    QWheelEvent wheel(QPointF(480.5, 300.25),
                      QPointF(480.5, 300.25),
                      QPoint(),
                      QPoint(0, 60),
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::NoScrollPhase,
                      false);
#else


    QWheelEvent wheel(QPointF(480.5, 300.25),
                      QPointF(480.5, 300.25),
                      QPoint(),
                      QPoint(0, 60),
                      60,
                      Qt::Vertical,
                      Qt::NoButton,
                      Qt::NoModifier);
#endif
    QApplication::sendEvent(&widget, &wheel);
    if (!(widget.wheel_events() == 1 && widget.last_wheel_delta() == 1.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)


    QWheelEvent pixels(QPointF(480.5, 300.25),
                       QPointF(480.5, 300.25),
                       QPoint(0, 8),
                       QPoint(),
                       Qt::NoButton,
                       Qt::NoModifier,
                       Qt::ScrollUpdate,
                       false);
#else


    QWheelEvent pixels(QPointF(480.5, 300.25),
                       QPointF(480.5, 300.25),
                       QPoint(0, 8),
                       QPoint(),
                       0,
                       Qt::Vertical,
                       Qt::NoButton,
                       Qt::NoModifier);
#endif
    QApplication::sendEvent(&widget, &pixels);
    if (!(widget.wheel_events() == 1 && !pixels.isAccepted()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    marker(widget, DEMO_RELEASED_RED, DEMO_RELEASED_GREEN);
    widget.close();
    return 0;
}
