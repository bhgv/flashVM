/* Real GTK3 SDK adapter fixture; requires caller-provided native display. */
#define main gtk_tutorial_entry
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/gtk3/main.c"

#undef main


int main(void)
{
    GtkHost host;
    GtkWidget *window;
    GdkEvent event;
    double x, y;
    int previous;
    memset(&host, 0, sizeof(host));
    if (!(gtk_init_check(NULL, NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_open(&host.demo) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_object_ref_sink(window);
    host.area = gtk_drawing_area_new();
    gtk_widget_set_size_request(host.area, 640, 400);
    gtk_container_add(GTK_CONTAINER(window), host.area);
    g_signal_connect(host.area, "draw", G_CALLBACK(gtk_draw), &host);
    gtk_widget_show_all(window);
    while (!host.demo.frames)
    {
        gtk_main_iteration_do(TRUE);
    }
    if (!(gtk_widget_get_allocated_width(host.area) == 640))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gtk_widget_get_allocated_height(host.area) == 400))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host.cursor_applied = -1;
    if (!(gtk_cursor(&host) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gdk_window_get_cursor(gtk_widget_get_window(host.area)) == NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(host.demo.context, 0, &previous) == FVM_OK && previous == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gtk_cursor(&host) == FVM_OK && host.hidden_cursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gdk_window_get_cursor(gtk_widget_get_window(host.area)) == host.hidden_cursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gdk_cursor_get_cursor_type(host.hidden_cursor) == GDK_BLANK_CURSOR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gtk_cursor(&host) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(host.demo.context, 1, &previous) == FVM_OK && previous == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gtk_cursor(&host) == FVM_OK && !gdk_window_get_cursor(gtk_widget_get_window(host.area))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&event, 0, sizeof(event));
    event.type = GDK_MOTION_NOTIFY;
    event.motion.x = 480.5;
    event.motion.y = 300.25;
    if (!(gtk_pointer(host.area, &event, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_pointer(host.demo.context, &x, &y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(x == 240.25 && y == 150.125))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.type = GDK_BUTTON_PRESS;
    event.button.button = 1;
    event.button.x = 200;
    event.button.y = 100;
    if (!(gtk_pointer(host.area, &event, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.type = GDK_2BUTTON_PRESS;
    if (!(!gtk_pointer(host.area, &event, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gtk_focus(window, NULL, &host);
    if (!(!host.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&event, 0, sizeof(event));
    event.type = GDK_SCROLL;
    event.scroll.x = 480.5;
    event.scroll.y = 300.25;
    event.scroll.direction = GDK_SCROLL_UP;
    if (!(gtk_scroll(host.area, &event.scroll, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.demo.wheel_events == 1 && host.demo.last_wheel_delta == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_pointer(host.demo.context, &x, &y) == FVM_OK && x == 240.25 && y == 150.125))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.scroll.direction = GDK_SCROLL_SMOOTH;
    event.scroll.delta_y = 0.25;
    if (!(gtk_scroll(host.area, &event.scroll, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.demo.wheel_events == 2 && host.demo.last_wheel_delta == -0.75))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.scroll.delta_y = 0;
    if (!(!gtk_scroll(host.area, &event.scroll, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.scroll.direction = GDK_SCROLL_LEFT;
    if (!(!gtk_scroll(host.area, &event.scroll, &host)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.demo.wheel_events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.demo.status == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gtk_widget_destroy(window);
    g_object_unref(window);
    if (host.hidden_cursor)
    {
        g_object_unref(host.hidden_cursor);
    }
    demo_close(&host.demo);
    return 0;
}
