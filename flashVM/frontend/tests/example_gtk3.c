/* Real GTK3 SDK adapter fixture; requires caller-provided native display. */
#define main gtk_tutorial_entry
#include "../../examples/frontends/gtk3/main.c"
#undef main
#include <assert.h>
int main(void)
{
    GtkHost host;
    GtkWidget *window;
    GdkEvent event;
    double x,y;
    int previous;
    memset(&host,0,sizeof(host));
    assert(gtk_init_check(NULL,NULL));
    assert(demo_open(&host.demo)==FVM_OK);
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL); g_object_ref_sink(window);
    host.area=gtk_drawing_area_new();
    gtk_widget_set_size_request(host.area,640,400);
    gtk_container_add(GTK_CONTAINER(window),host.area);
    g_signal_connect(host.area,"draw",G_CALLBACK(gtk_draw),&host);
    gtk_widget_show_all(window);
    while(!host.demo.frames) gtk_main_iteration_do(TRUE);
    assert(gtk_widget_get_allocated_width(host.area)==640);
    assert(gtk_widget_get_allocated_height(host.area)==400);
    host.cursor_applied=-1;
    assert(gtk_cursor(&host)==FVM_OK);
    assert(gdk_window_get_cursor(gtk_widget_get_window(host.area))==NULL);
    assert(fvm_input_set_cursor_visible(host.demo.context,0,&previous)==FVM_OK && previous==1);
    assert(gtk_cursor(&host)==FVM_OK && host.hidden_cursor);
    assert(gdk_window_get_cursor(gtk_widget_get_window(host.area))==host.hidden_cursor);
    assert(gdk_cursor_get_cursor_type(host.hidden_cursor)==GDK_BLANK_CURSOR);
    assert(gtk_cursor(&host)==FVM_OK);
    assert(fvm_input_set_cursor_visible(host.demo.context,1,&previous)==FVM_OK && previous==0);
    assert(gtk_cursor(&host)==FVM_OK && !gdk_window_get_cursor(gtk_widget_get_window(host.area)));
    memset(&event,0,sizeof(event)); event.type=GDK_MOTION_NOTIFY;
    event.motion.x=480.5; event.motion.y=300.25;
    assert(gtk_pointer(host.area,&event,&host));
    assert(fvm_input_get_pointer(host.demo.context,&x,&y)==FVM_OK);
    assert(x==240.25 && y==150.125);
    event.type=GDK_BUTTON_PRESS; event.button.button=1;
    event.button.x=200; event.button.y=100;
    assert(gtk_pointer(host.area,&event,&host)); assert(host.demo.held);
    event.type=GDK_2BUTTON_PRESS;
    assert(!gtk_pointer(host.area,&event,&host));
    gtk_focus(window,NULL,&host); assert(!host.demo.held);
    memset(&event,0,sizeof(event));event.type=GDK_SCROLL;
    event.scroll.x=480.5;event.scroll.y=300.25;event.scroll.direction=GDK_SCROLL_UP;
    assert(gtk_scroll(host.area,&event.scroll,&host));
    assert(host.demo.wheel_events==1 && host.demo.last_wheel_delta==3);
    assert(fvm_input_get_pointer(host.demo.context,&x,&y)==FVM_OK && x==240.25 && y==150.125);
    event.scroll.direction=GDK_SCROLL_SMOOTH;event.scroll.delta_y=0.25;
    assert(gtk_scroll(host.area,&event.scroll,&host));
    assert(host.demo.wheel_events==2 && host.demo.last_wheel_delta==-0.75);
    event.scroll.delta_y=0;assert(!gtk_scroll(host.area,&event.scroll,&host));
    event.scroll.direction=GDK_SCROLL_LEFT;assert(!gtk_scroll(host.area,&event.scroll,&host));
    assert(host.demo.wheel_events==2);
    assert(host.demo.status==FVM_OK);
    gtk_widget_destroy(window); g_object_unref(window);
    if(host.hidden_cursor)g_object_unref(host.hidden_cursor);
    demo_close(&host.demo);
    return 0;
}
