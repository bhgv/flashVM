/* Real Qt5 SDK paint/input fixture; offscreen is software acceptance only. */
#define main qt_tutorial_entry
#include "../../examples/frontends/qt5/main.cpp"
#undef main
#include <cassert>
static void marker(DemoWidget &widget,int r,int g)
{
    QImage frame=widget.grab().toImage();
    assert(!frame.isNull());
    QRgb pixel=frame.pixel(480,300);
    assert(qRed(pixel)==r && qGreen(pixel)==g && qBlue(pixel)==DEMO_MARKER_BLUE);
}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    DemoWidget widget(false);
    assert(widget.status()==FVM_OK);
    widget.resize(640,400); widget.show(); app.processEvents();
    QMouseEvent move(QEvent::MouseMove,QPointF(480.5,300.25),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&widget,&move); marker(widget,DEMO_RELEASED_RED,DEMO_RELEASED_GREEN);
    QMouseEvent down(QEvent::MouseButtonPress,QPointF(480.5,300.25),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&widget,&down); marker(widget,DEMO_HELD_RED,DEMO_HELD_GREEN);
    QEvent deactivate(QEvent::WindowDeactivate);
    QApplication::sendEvent(&widget,&deactivate); marker(widget,DEMO_RELEASED_RED,DEMO_RELEASED_GREEN);
    assert(widget.status()==FVM_OK && widget.frames()>=3);
    widget.close();
    return 0;
}
