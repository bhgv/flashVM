#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b, unsigned int n) { b.push_back((unsigned char)n); b.push_back((unsigned char)(n >> 8)); }
static void name(Bytes &b, const char *s) { do { b.push_back((unsigned char)*s); } while (*s++); }
static void text(Bytes &b, const char *s) { b.push_back(0x96); u16(b, (unsigned int)std::strlen(s) + 2); b.push_back(0); name(b, s); }
static void variable(Bytes &b, const char *s) { text(b, s); b.push_back(0x1c); }
static void integer(Bytes &b, unsigned int n)
{
    b.push_back(0x96); u16(b, 5); b.push_back(7);
    for (unsigned int i = 0; i < 4; ++i) { b.push_back((unsigned char)n); n >>= 8; }
}
static void reg(Bytes &b, unsigned int n) { b.push_back(0x96); u16(b, 2); b.push_back(4); b.push_back((unsigned char)n); }
static FVM_Value define(FVM_Context *c, const char *symbol, unsigned int count, unsigned int flags, const Bytes &body)
{
    Bytes header, code; name(header, symbol); u16(header, 0); header.push_back((unsigned char)count);
    u16(header, flags); u16(header, (unsigned int)body.size()); code.push_back(0x8e);
    u16(code, (unsigned int)header.size());
    for (size_t i = 0; i < header.size(); ++i) code.push_back(header[i]);
    for (size_t i = 0; i < body.size(); ++i) code.push_back(body[i]);
    code.push_back(0); assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
    FVM_Value out; assert(fvm_get_global(c, symbol, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); return out;
}
static void extend(FVM_Context *c, const char *sub, const char *base)
{
    Bytes b; variable(b, sub); variable(b, base); b.push_back(0x69); b.push_back(0);
    assert(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_OK);
}
static FVM_Object *prototype(FVM_Context *c, const FVM_Value &f)
{
    FVM_Value out; assert(fvm_object_get(c, f.object, "prototype", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); return out.object;
}
static void result(Bytes &b) { b.push_back(0x3e); b.push_back(0); }
static FVM_Result getter(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{ return fvm_object_get(c, self, "marker", out); }
static FVM_Result setter(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a, FVM_Value *out, void *)
{
    assert(n == 1); FVM_Result r = fvm_object_set(c, self, "marker", a); fvm_value_undefined(out); return r;
}
int main()
{
    FVM_Config config; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&config) == FVM_OK); assert(fvm_runtime_create(&config, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    Bytes base_body; variable(base_body, "this"); text(base_body, "marker"); integer(base_body, 123);
    base_body.push_back(0x4f); base_body.push_back(0);
    FVM_Value base = define(c, "Base", 0, 32, base_body);
    Bytes child_body; integer(child_body, 0); variable(child_body, "super"); text(child_body, "");
    child_body.push_back(0x52); child_body.push_back(0x17); child_body.push_back(0);
    FVM_Value sub = define(c, "Sub", 0, 0, child_body); extend(c, "Sub", "Base");
    Bytes read_body; variable(read_body, "this"); text(read_body, "marker"); read_body.push_back(0x4e); result(read_body);
    FVM_Value read = define(c, "read", 0, 32, read_body);
    assert(fvm_object_set(c, prototype(c, base), "read", &read) == FVM_OK);
    Bytes foo_body; integer(foo_body, 0); reg(foo_body, 1); text(foo_body, "read"); foo_body.push_back(0x52); result(foo_body);
    FVM_Value foo = define(c, "foo", 1, 16, foo_body);
    assert(fvm_object_set(c, prototype(c, sub), "foo", &foo) == FVM_OK);
    FVM_Object *instance = 0;
    assert(fvm_function_construct(c, sub.object, 0, 0, &instance) == FVM_OK);
    assert(fvm_gc_pin(c, instance) == FVM_OK);
    FVM_Value out; assert(fvm_object_get(c, instance, "marker", &out) == FVM_OK && out.number == 123);
    assert(fvm_object_call(c, instance, "foo", 0, 0, &out) == FVM_OK && out.number == 123);
    Bytes override_body; integer(override_body, 900); result(override_body);
    FVM_Value override_read = define(c, "overrideRead", 0, 32, override_body);
    assert(fvm_object_set(c, prototype(c, sub), "read", &override_read) == FVM_OK);
    FVM_Value third = define(c, "Third", 0, 0, child_body); extend(c, "Third", "Sub");
    FVM_Object *third_instance = 0;
    assert(fvm_function_construct(c, third.object, 0, 0, &third_instance) == FVM_OK);
    assert(fvm_gc_pin(c, third_instance) == FVM_OK);
    assert(fvm_object_call(c, third_instance, "read", 0, 0, &out) == FVM_OK && out.number == 900);
    assert(fvm_object_call(c, third_instance, "foo", 0, 0, &out) == FVM_OK && out.number == 123);
    Bytes suppressed_body; variable(suppressed_body, "super"); result(suppressed_body);
    FVM_Value suppressed = define(c, "suppressed", 0, 32, suppressed_body);
    assert(fvm_object_set(c, prototype(c, sub), "suppressed", &suppressed) == FVM_OK);
    assert(fvm_object_call(c, instance, "suppressed", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    Bytes paired_body; reg(paired_body, 1); result(paired_body);
    FVM_Value paired = define(c, "paired", 1, 48, paired_body);
    assert(fvm_object_set(c, prototype(c, sub), "paired", &paired) == FVM_OK);
    assert(fvm_object_call(c, instance, "paired", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    Bytes throwing_body; text(throwing_body, "parent failure"); throwing_body.push_back(0x2a); throwing_body.push_back(0);
    FVM_Value throwing = define(c, "throwing", 0, 32, throwing_body);
    assert(fvm_object_set(c, prototype(c, base), "throwing", &throwing) == FVM_OK);
    Bytes caught_body; integer(caught_body, 0); variable(caught_body, "super"); text(caught_body, "throwing");
    caught_body.push_back(0x52); result(caught_body);
    FVM_Value fault = define(c, "fault", 0, 0, caught_body);
    assert(fvm_object_set(c, prototype(c, sub), "fault", &fault) == FVM_OK);
    assert(fvm_object_call(c, instance, "fault", 0, 0, &out) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "parent failure"));
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_object_call(c, instance, "foo", 0, 0, &out) == FVM_OK && out.number == 123);
    Bytes escaped_body; variable(escaped_body, "super"); result(escaped_body);
    FVM_Value escaped = define(c, "escape", 0, 0, escaped_body);
    assert(fvm_object_set(c, prototype(c, sub), "escape", &escaped) == FVM_OK);
    assert(fvm_object_call(c, instance, "escape", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK);
    Bytes replacement_body; replacement_body.push_back(0);
    FVM_Value replacement = define(c, "Replacement", 0, 32, replacement_body);
    assert(fvm_object_set(c, prototype(c, replacement), "read", &override_read) == FVM_OK);
    assert(fvm_object_set_prototype(c, prototype(c, sub), prototype(c, replacement)) == FVM_OK);
    assert(fvm_object_call(c, facade, "read", 0, 0, &out) == FVM_OK && out.number == 900);
    assert(fvm_object_set_prototype(c, prototype(c, sub), prototype(c, base)) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, facade, "read", 0, 0, &out) == FVM_OK && out.number == 123);
    FVM_Value message; fvm_value_undefined(&message); message.type = FVM_VALUE_OBJECT; message.object = facade;
    FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &message, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &message, 0) == FVM_OK);
    assert(fvm_object_call(other, message.object, "read", 0, 0, &out) == FVM_OK && out.number == 123);
    assert(fvm_object_define_accessor_rooted(c, prototype(c, base), "field", getter, setter, 0, 0) == FVM_OK);
    assert(fvm_object_get(c, facade, "field", &out) == FVM_OK && out.number == 123);
    fvm_value_number(&out, 456);
    assert(fvm_object_set(c, facade, "field", &out) == FVM_OK);
    assert(fvm_object_get(c, instance, "marker", &out) == FVM_OK && out.number == 456);
    assert(fvm_gc_unpin(c, third_instance) == FVM_OK);
    assert(fvm_gc_unpin(c, facade) == FVM_OK); assert(fvm_gc_unpin(c, instance) == FVM_OK);
    fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
