#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
static void coordinate(FVM_Context *c, FVM_Object *clip, const char *method, double x, double y)
{
    FVM_Value args[2], out;
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    assert(fvm_object_call(c, clip, method, 2, args, &out) == FVM_OK);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0, *child = 0, *point = 0;
    FVM_Value args[3], out, value, copy, klass;
    FVM_Channel *ch = 0;
    int matches;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_create(c, "root", 3, &root) == FVM_OK);
    fvm_value_string(args, "child");
    fvm_value_number(args + 1, 4);
    assert(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK);
    child = out.object;
    assert(fvm_object_get(c, child, "_parent", &out) == FVM_OK && out.object == root);
    assert(fvm_object_get(c, child, "_root", &out) == FVM_OK && out.object == root);
    assert(fvm_object_call(c, child, "toString", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, "/root/child"));
    assert(fvm_object_call(c, root, "getNextHighestDepth", 0, 0, &out) == FVM_OK &&
           out.number == 5);
    fvm_value_number(&value, 10);
    assert(fvm_object_set(c, root, "_x", &value) == FVM_OK);
    fvm_value_number(&value, 20);
    assert(fvm_object_set(c, child, "_y", &value) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &point) == FVM_OK);
    fvm_value_number(&value, 1);
    assert(fvm_object_set(c, point, "x", &value) == FVM_OK);
    fvm_value_number(&value, 2);
    assert(fvm_object_set(c, point, "y", &value) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = point;
    assert(fvm_object_call(c, child, "localToGlobal", 1, &value, &out) == FVM_OK);
    assert(fvm_object_get(c, point, "x", &out) == FVM_OK && out.number == 11);
    assert(fvm_object_get(c, point, "y", &out) == FVM_OK && out.number == 22);
    assert(fvm_object_call(c, child, "globalToLocal", 1, &value, &out) == FVM_OK);
    assert(fvm_object_get(c, point, "x", &out) == FVM_OK && out.number == 1);
    fvm_value_number(args, 16711680);
    assert(fvm_object_call(c, child, "beginFill", 1, args, &out) == FVM_OK);
    coordinate(c, child, "moveTo", 0, 0);
    coordinate(c, child, "lineTo", 10, 0);
    coordinate(c, child, "lineTo", 10, 10);
    coordinate(c, child, "lineTo", 0, 10);
    coordinate(c, child, "lineTo", 0, 0);
    assert(fvm_object_call(c, child, "endFill", 0, 0, &out) == FVM_OK);
    assert(fvm_object_call(c, child, "getBounds", 0, 0, &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "xMax", &value) == FVM_OK && value.number == 10);
    assert(fvm_movieclip_define_frame(c, root, 2, "second", 0, 0) == FVM_OK);
    fvm_value_string(args, "second");
    assert(fvm_object_call(c, root, "gotoAndStop", 1, args, &out) == FVM_OK);
    assert(fvm_object_get(c, root, "_currentframe", &out) == FVM_OK && out.number == 2);
    assert(fvm_movieclip_advance(c, root, 2) == FVM_OK);
    assert(fvm_object_get(c, root, "_currentframe", &out) == FVM_OK && out.number == 2);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = root;
    assert(fvm_object_set(c, root, "self", &value) == FVM_OK);
    {
        const unsigned char stop_action[] = {0x07, 0};
        const unsigned char root_action[] = {0x96, 6, 0,   0,   's', 'e', 'e', 'n', 0,    0x96, 7,
                                             0,    0, '_', 'r', 'o', 'o', 't', 0,   0x1c, 0x1d, 0};
        assert(fvm_movieclip_define_frame(c, root, 3, "last", stop_action, sizeof(stop_action)) ==
               FVM_OK);
        assert(fvm_movieclip_define_frame(c, child, 1, "start", root_action, sizeof(root_action)) ==
               FVM_OK);
    }
    assert(fvm_channel_create(&ch) == FVM_OK);
    assert(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    assert(fvm_object_get(other, copy.object, "child", &out) == FVM_OK &&
           out.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(other, out.object, "_parent", &value) == FVM_OK &&
           value.object == copy.object);
    assert(fvm_object_get(other, copy.object, "_currentframe", &value) == FVM_OK &&
           value.number == 2);
    assert(fvm_object_get(other, copy.object, "constructor", &klass) == FVM_OK);
    assert(fvm_value_instance_of(other, &copy, &klass, &matches) == FVM_OK && matches);
    assert(fvm_object_call(other, out.object, "getBounds", 0, 0, &value) == FVM_OK);
    assert(fvm_object_get(other, value.object, "xMax", &value) == FVM_OK && value.number == 10);
    fvm_value_number(args, 3);
    assert(fvm_object_call(other, copy.object, "gotoAndPlay", 1, args, &value) == FVM_OK);
    assert(fvm_movieclip_advance(other, copy.object, 1) == FVM_OK);
    assert(fvm_movieclip_advance(other, copy.object, 3) == FVM_OK);
    assert(fvm_object_get(other, copy.object, "_currentframe", &value) == FVM_OK &&
           value.number == 3);
    assert(fvm_object_get(other, copy.object, "child", &out) == FVM_OK);
    assert(fvm_object_get(other, out.object, "seen", &value) == FVM_OK &&
           value.object == copy.object);
    fvm_channel_destroy(ch);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
