#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void word(Bytes &b, size_t n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}
static void text(Bytes &b, const char *s)
{
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}
static void string(Bytes &b, const char *s)
{
    b.push_back(0x96);
    word(b, std::strlen(s) + 2);
    b.push_back(0);
    text(b, s);
}
static void integer(Bytes &b, unsigned long n)
{
    b.push_back(0x96);
    word(b, 5);
    b.push_back(7);
    for (int i = 0; i < 4; ++i)
        b.push_back((unsigned char)(n >> (i * 8)));
}
static void variable(Bytes &b, const char *s)
{
    string(b, s);
    b.push_back(0x1c);
}
static void assign(Bytes &b, const char *name, unsigned long n)
{
    string(b, name);
    integer(b, n);
    b.push_back(0x1d);
}
static void append(Bytes &b, const Bytes &source)
{
    for (size_t i = 0; i < source.size(); ++i)
        b.push_back(source[i]);
}
static void attempt(Bytes &b, const Bytes &body, const Bytes &caught, const Bytes &final,
                    unsigned int flags, const char *name)
{
    b.push_back(0x8f);
    word(b, 7 + std::strlen(name) + 1);
    b.push_back((unsigned char)flags);
    word(b, body.size());
    word(b, caught.size());
    word(b, final.size());
    text(b, name);
    append(b, body);
    append(b, caught);
    append(b, final);
}
static void run(FVM_Context *c, Bytes b, FVM_Result expected)
{
    b.push_back(0);
    assert(fvm_avm1_execute(c, &b[0], b.size()) == expected);
}
static double global(FVM_Context *c, const char *name)
{
    FVM_Value v;
    assert(fvm_get_global(c, name, &v) == FVM_OK);
    assert(v.type == FVM_VALUE_NUMBER);
    return v.number;
}
static void method(Bytes &b, const char *object, const char *name)
{
    integer(b, 0);
    variable(b, object);
    string(b, name);
    b.push_back(0x52);
}
static void define(Bytes &b, const char *name, const Bytes &body)
{
    b.push_back(0x9b);
    word(b, std::strlen(name) + 1 + 2 + 2 + body.size());
    text(b, name);
    word(b, 0);
    word(b, body.size());
    append(b, body);
}
static FVM_Result object_result(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *,
                                FVM_Value *out, void *)
{
    if (n)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *object = 0;
    FVM_Result result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;
    FVM_Value marker;
    fvm_value_number(&marker, 73);
    result = fvm_object_set(c, object, "marker", &marker);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return result;
}
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
    FVM_Value v, lock, namespace_value;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_extensions_install(c) == FVM_OK);
    /* Named catch receives arbitrary thrown values; finally always runs. */
    Bytes body, caught, final, code;
    integer(body, 42);
    body.push_back(0x2a);
    string(caught, "observed");
    variable(caught, "error");
    caught.push_back(0x1d);
    assign(final, "finished", 1);
    attempt(code, body, caught, final, 3, "error");
    run(c, code, FVM_OK);
    assert(global(c, "observed") == 42);
    assert(global(c, "finished") == 1);
    /* A finally throw replaces an earlier uncaught throw. */
    Bytes replacement, empty, outer;
    integer(replacement, 99);
    replacement.push_back(0x2a);
    attempt(outer, body, empty, replacement, 2, "");
    run(c, outer, FVM_EXCEPTION);
    assert(fvm_exception_get(c, &v) == FVM_OK && v.number == 99);
    assert(fvm_exception_clear(c) == FVM_OK);
    /* A caught rethrow is caught by the enclosing record. */
    Bytes rethrow, inner, nested;
    variable(rethrow, "inner");
    rethrow.push_back(0x2a);
    attempt(inner, body, rethrow, empty, 1, "inner");
    attempt(nested, inner, caught, empty, 1, "error");
    run(c, nested, FVM_OK);
    assert(global(c, "observed") == 42);
    /* CatchInRegister uses the same register bank as the enclosing frame. */
    Bytes register_catch, register_try;
    string(register_catch, "registered");
    register_catch.push_back(0x96);
    word(register_catch, 2);
    register_catch.push_back(4);
    register_catch.push_back(1);
    register_catch.push_back(0x1d);
    register_try.push_back(0x8f);
    word(register_try, 8);
    register_try.push_back(5);
    word(register_try, body.size());
    word(register_try, register_catch.size());
    word(register_try, 0);
    register_try.push_back(1);
    append(register_try, body);
    append(register_try, register_catch);
    run(c, register_try, FVM_OK);
    assert(global(c, "registered") == 42);
    /* Return runs finally; an explicit finally return supersedes it. */
    Bytes return_body, return_final, return_try, definitions;
    integer(return_body, 7);
    return_body.push_back(0x3e);
    attempt(return_try, return_body, empty, final, 2, "");
    return_try.push_back(0);
    define(definitions, "returnThroughFinally", return_try);
    integer(return_final, 9);
    return_final.push_back(0x3e);
    Bytes override_try;
    attempt(override_try, return_body, empty, return_final, 2, "");
    override_try.push_back(0);
    define(definitions, "overrideReturn", override_try);
    run(c, definitions, FVM_OK);
    assert(fvm_avm1_call(c, "returnThroughFinally", 0, 0, 0, &v) == FVM_OK && v.number == 7);
    assert(fvm_avm1_call(c, "overrideReturn", 0, 0, 0, &v) == FVM_OK && v.number == 9);
    /* Closed resource exceptions carry metadata and survive full GC. */
    assert(fvm_get_global(c, "mutex", &namespace_value) == FVM_OK);
    assert(fvm_object_call(c, namespace_value.object, "new", 0, 0, &lock) == FVM_OK);
    assert(fvm_set_global(c, "lock", &lock) == FVM_OK);
    assert(fvm_object_call(c, lock.object, "close", 0, 0, &v) == FVM_OK);
    assert(fvm_object_call(c, lock.object, "touch", 0, 0, &v) == FVM_EXCEPTION);
    FVM_ExceptionInfo info;
    assert(fvm_exception_info(c, &info) == FVM_OK && info.code == FVM_RESOURCE_CLOSED);
    assert(!std::strcmp(info.kind, "mutex"));
    assert(info.resource != 0);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_exception_get(c, &v) == FVM_OK);
    FVM_Value code_value;
    assert(fvm_object_get(c, v.object, "code", &code_value) == FVM_OK && code_value.number == 1001);
    assert(fvm_exception_clear(c) == FVM_OK);
    Bytes failed, handler, handled;
    method(failed, "lock", "touch");
    string(handler, "caughtResource");
    variable(handler, "failure");
    handler.push_back(0x1d);
    attempt(handled, failed, handler, final, 3, "failure");
    run(c, handled, FVM_OK);
    assert(fvm_get_global(c, "caughtResource", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, v.object, "code", &code_value) == FVM_OK && code_value.number == 1001);
    assert(fvm_exception_get(c, &v) == FVM_NOT_FOUND);
    /* Pending exceptions survive an explicit collection inside finally. */
    Bytes collecting, uncaught, throw_definition, cross_call, cross_handler;
    method(collecting, "gc", "collect");
    collecting.push_back(0x17);
    attempt(uncaught, failed, empty, collecting, 2, "");
    run(c, uncaught, FVM_EXCEPTION);
    assert(fvm_exception_get(c, &v) == FVM_OK && v.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, v.object, "code", &code_value) == FVM_OK && code_value.number == 1001);
    assert(fvm_exception_clear(c) == FVM_OK);
    failed.push_back(0);
    define(throw_definition, "resourceFailure", failed);
    run(c, throw_definition, FVM_OK);
    integer(cross_call, 0);
    string(cross_call, "resourceFailure");
    cross_call.push_back(0x3d);
    attempt(cross_handler, cross_call, handler, empty, 1, "failure");
    run(c, cross_handler, FVM_OK);
    /* An unpinned return object remains alive after finally discards the
     * operand and collects: only the saved completion holds that reference. */
    assert(fvm_register_function(c, "newObject", object_result, 0) == FVM_OK);
    Bytes object_body, object_final, object_try, object_definition;
    integer(object_body, 0);
    string(object_body, "newObject");
    object_body.push_back(0x3d);
    object_body.push_back(0x3e);
    object_final.push_back(0x17);
    append(object_final, collecting);
    attempt(object_try, object_body, empty, object_final, 2, "");
    object_try.push_back(0);
    define(object_definition, "objectReturn", object_try);
    run(c, object_definition, FVM_OK);
    assert(fvm_avm1_call(c, "objectReturn", 0, 0, 0, &v) == FVM_OK && v.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, v.object, "marker", &code_value) == FVM_OK && code_value.number == 73);
    /* Invalid embedded action blocks are rejected before any side effect. */
    Bytes invalid, malformed;
    assign(invalid, "mustNotRun", 1);
    malformed.push_back(0x96);
    attempt(invalid, malformed, empty, empty, 0, "");
    run(c, invalid, FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(c, "mustNotRun", &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}
