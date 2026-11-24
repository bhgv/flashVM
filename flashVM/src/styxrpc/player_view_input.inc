/* Copyright (c) 2026 bhgv
 * EN: Owner-thread poll interface for copied native SDL events.
 * RU: Опрос копий событий SDL в потоке владельца фильма.
 */
#ifdef FVM_STYXRPC_WINDOW

/* EN: Store a copied numeric field on the movie's rooted input object.
 * RU: Сохранить числовое поле в удерживаемом объекте ввода фильма. */
static FVM_Result input_number(FVM_Context *c, FVM_Object *receiver, const char *name, double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_object_set(c, receiver, name, &value);
}

#include "player_view_resize.inc"
#include "player_view_frame.inc"

/* EN: Opt the current movie into polling. The SDL owner applies the change on its next turn.
 * RU: Включить опрос для фильма. Владелец SDL применит его на следующем шаге. */
static FVM_Result enable_input(FVM_Context *c, FVM_Object *receiver, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)args;
    (void)state;
    fvm_value_boolean(out, 1);
    return count ? FVM_INVALID_ARGUMENT : fvm_object_set(c, receiver, "enabled", out);
}


/* EN: Read one member of the last copied event by its counted name.
 * RU: Прочитать поле последнего скопированного события по имени с длиной. */
static FVM_Result input_field(FVM_Context *c, FVM_Object *receiver, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)state;
    if (count != 1 || args[0].type != FVM_VALUE_STRING)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_object_get_bytes(c, receiver, args[0].string, fvm_value_string_size(args), out);
}


/* EN: Pop at most one native event. Snapshot fields remain readable until the next successful take.
 * RU: Снять не более одного события. Поля снимка доступны до следующего успешного вызова. */
static FVM_Result take_input(FVM_Context *c, FVM_Object *receiver, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_PairedView *view = state;
    FVM_DesktopHostEvent event;
    FVM_Result result;
    const char *names[] = {"code", "phase", "x", "y", "wheel", "control"};
    double numbers[sizeof(names) / sizeof(names[0])];
    size_t i;
    (void)args;
    if (count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!fvm_desktop_window_take_input(view->window, &event))
    {
        fvm_value_number(out, 0);
        return FVM_OK;
    }
    numbers[0] = event.code;
    numbers[1] = event.phase;
    numbers[2] = event.x;
    numbers[3] = event.y;
    numbers[4] = event.wheel;
    numbers[5] = event.control;
    result = FVM_OK;
    for (i = 0; result == FVM_OK && i < sizeof(names) / sizeof(names[0]); ++i)
    {
        result = input_number(c, receiver, names[i], numbers[i]);
    }
    if (result == FVM_OK)
    {
        FVM_Value bytes, text;
        unsigned short units[FVM_DESKTOP_INPUT_UTF8];
        size_t used;
        result = fvm_value_string(&bytes, event.text);
        if (result == FVM_OK)
        {
            result = fvm_value_get_utf16(&bytes, units, sizeof(units) / sizeof(units[0]), &used);
        }
        if (result == FVM_OK)
        {
            result = fvm_value_string_utf16(c, units, used, &text);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_set(c, receiver, "text", &text);
        }
    }
    if (result == FVM_OK)
    {
        fvm_value_number(out, event.kind);
    }
    return result;
}


/* EN: Apply the opt-in for the committed scene. Clear old FIFO entries across movie replacement.
 * RU: Применить режим активного фильма. При замене сбросить очередь предыдущего фильма. */
static int update_input_owner(FVM_PairedView *view, FVM_Context *c)
{
    FVM_Value object, serial, enabled;
    int direct = 0;
    unsigned long owner = 0;
    if (fvm_get_global(c, "hostInput", &object) == FVM_OK && object.type == FVM_VALUE_OBJECT &&
        fvm_object_get(c, object.object, "serial", &serial) == FVM_OK && serial.type == FVM_VALUE_NUMBER &&
        serial.number >= 1 && serial.number <= (double)view->input_serial)
    {
        owner = (unsigned long)serial.number;
        direct = fvm_object_get(c, object.object, "enabled", &enabled) == FVM_OK &&
            enabled.type == FVM_VALUE_BOOLEAN && enabled.boolean;
    }
    if (owner != view->input_owner)
    {
        if (!fvm_desktop_window_follow_size(view->window, 0, 0, 0, 0))
        {
            return -1;
        }
        fvm_desktop_window_host_input(view->window, 0);
        view->input_owner = owner;
        view->painted_revision = 0;
    }
    fvm_desktop_window_host_input(view->window, direct);
    return direct;
}
#endif


/* [GC:] EN/RU: See player_view.h. / См. player_view.h. */
FVM_Result fvm_paired_view_install_input(FVM_PairedView *view, FVM_Context *context)
{
#ifdef FVM_STYXRPC_WINDOW
    if (view && view->windowed)
    {
        FVM_GCRootScope *roots;
        FVM_Value object;
        FVM_Result result = fvm_gc_roots_begin(context, &roots);
        if (result != FVM_OK)
        {
            return result;
        }
        fvm_value_undefined(&object);
        result = fvm_object_create(context, &object.object);
        if (result == FVM_OK)
        {
            object.type = FVM_VALUE_OBJECT;
            result = fvm_gc_roots_add(roots, &object);
        }
        if (result == FVM_OK)
        {
            result = input_number(context, object.object, "serial", (double)++view->input_serial);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "enable", enable_input, view, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "take", take_input, view, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "get", input_field, view, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "enableResize", enable_resize, NULL, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "commitViewport", commit_viewport,
                NULL, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_object_define_method_rooted(context, object.object, "publishFrame", publish_frame, NULL, NULL);
        }
        if (result == FVM_OK)
        {
            result = fvm_set_global(context, "hostInput", &object);
        }
        fvm_gc_roots_end(roots);
        return result;
    }
#else
    (void)view;
    (void)context;
#endif
    return FVM_OK;
}
