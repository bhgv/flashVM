/* Shared AVM1 oracle inputs for current-global primitive boxing regressions. */
#ifndef FVM_TEST_PRIMITIVE_BOXING_FIXTURE_HPP
#define FVM_TEST_PRIMITIVE_BOXING_FIXTURE_HPP
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/serialization.h"
#include "flashvm/table.h"

struct BoxingFixture
{
    FVM_Context *context;
    const char *name;
    FVM_Value input;
    FVM_Object *original;
    FVM_Object *prototype;
    FVM_Object *factory;
    FVM_Object *failure;
    unsigned version, type, kind, reads;
    unsigned collections, blocked_collections;
};

static void boxing_equal(const FVM_Value &a, const FVM_Value &b)
{
    assert(a.type == b.type);
    if (a.type == FVM_VALUE_NUMBER) assert(a.number == b.number);
    if (a.type == FVM_VALUE_BOOLEAN) assert(a.boolean == b.boolean);
    if (a.type == FVM_VALUE_OBJECT) assert(a.object == b.object);
    if (a.type == FVM_VALUE_STRING)
        assert(fvm_value_string_size(&a) == fvm_value_string_size(&b) &&
               !std::memcmp(a.string, b.string, fvm_value_string_size(&a)));
}

static FVM_Result boxing_getter(FVM_Context *c, const FVM_ScriptFrame *frame,
                                 FVM_Value *out, void *raw)
{
    BoxingFixture *fixture = (BoxingFixture *)raw;
    if (fixture->kind == 10 && (frame->argc != 1 || frame->argv[0].type != FVM_VALUE_STRING ||
                               std::strcmp(frame->argv[0].string, fixture->name)))
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    ++fixture->reads;
    if (c->gc_native_depth)
    {
        assert(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
        ++fixture->blocked_collections;
    }
    else
    {
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        ++fixture->collections;
    }
    if (fixture->kind == 8)
        return FVM_ERROR;
    if (fixture->kind == 9)
    {
        *out = class_value(fixture->failure);
        return fvm_exception_raise(c, out);
    }
    if (fixture->kind == 7)
        fvm_value_undefined(out);
    else
        *out = class_value(fixture->factory);
    return FVM_OK;
}

static void boxing_init(FVM_Runtime *runtime, BoxingFixture &f, unsigned version,
                         unsigned type, unsigned kind)
{
    const char *names[] = {"Number", "Boolean", "String"};
    f.context = 0;
    f.name = names[type]; f.version = version; f.type = type; f.kind = kind;
    f.reads = f.collections = f.blocked_collections = 0;
    assert(fvm_context_create(runtime, &f.context) == FVM_OK);
    FVM_Context *c = f.context;
    if (version)
    {
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    }
    assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
    FVM_Value value;
    assert(fvm_get_global(c, f.name, &value) == FVM_OK);
    f.original = value.object;
    assert(fvm_object_get(c, f.original, "prototype", &value) == FVM_OK && fvm_gc_pin(c, value.object) == FVM_OK);
    f.prototype = value.object;
    assert(fvm_object_create(c, &f.failure) == FVM_OK);
    value = class_value(f.failure);
    assert(fvm_object_set(c, f.failure, "cycle", &value) == FVM_OK && fvm_set_global(c, "boxingFailure", &value) == FVM_OK);
    Program body;
    if (kind == 13)
    {
        body.variable("boxingFailure"); body.op(0x2a); body.op(0);
    }
    else
    {
        body.variable("this"); body.text("boxed"); body.variable("x"); body.op(0x4f);
        body.variable("this"); body.finish();
    }
    const char *parameters[] = {"x"};
    assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &f.factory) == FVM_OK &&
           fvm_gc_pin(c, f.factory) == FVM_OK);
    if (type == 0) fvm_value_number(&f.input, 42);
    if (type == 1) fvm_value_boolean(&f.input, 0);
    if (type == 2)
    {
        assert(fvm_value_string_bytes(&f.input, "a\0b", 3) == FVM_OK);
    }
    assert(fvm_set_global(c, "boxingInput", &f.input) == FVM_OK);
    fvm_value_undefined(&value);
    if (kind == 2) value.type = FVM_VALUE_NULL;
    if (kind == 3) fvm_value_number(&value, 17);
    if (kind == 4)
    {
        FVM_Object *plain = 0;
        assert(fvm_object_create(c, &plain) == FVM_OK); value = class_value(plain);
    }
    if (kind == 5 || kind == 13) value = class_value(f.factory);
    if (kind && kind != 12)
        assert(fvm_set_global(c, f.name, &value) == FVM_OK);
    if (kind >= 6 && kind <= 10)
    {
        FVM_Object *getter = 0;
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK &&
               fvm_function_set_body(c, getter, boxing_getter, &f, f.factory) == FVM_OK);
        value = class_value(getter);
        if (kind == 10)
        {
            int removed = 0;
            assert(fvm_object_delete(c, c->global, f.name, &removed) == FVM_OK && removed &&
                   fvm_set_global(c, "__resolve", &value) == FVM_OK);
        }
        else
        {
            FVM_Value descriptor[3], object_class, prototype, method, ignored;
            assert(fvm_get_global(c, "Object", &object_class) == FVM_OK &&
                   fvm_object_get(c, object_class.object, "prototype", &prototype) == FVM_OK &&
                   fvm_object_get(c, prototype.object, "addProperty", &method) == FVM_OK);
            fvm_value_string(descriptor, f.name); descriptor[1] = value;
            fvm_value_undefined(descriptor + 2); descriptor[2].type = FVM_VALUE_NULL;
            assert(fvm_function_invoke(c, method.object, c->global, 3, descriptor, &ignored) == FVM_OK && ignored.boolean);
        }
    }
    if (kind == 11)
    {
        FVM_Object *carrier = 0;
        int removed = 0;
        value = class_value(f.factory);
        assert(fvm_object_create(c, &carrier) == FVM_OK &&
               fvm_object_set(c, carrier, f.name, &value) == FVM_OK &&
               fvm_object_set_prototype(c, carrier, c->global->prototype) == FVM_OK &&
               fvm_object_set_prototype(c, c->global, carrier) == FVM_OK &&
               fvm_object_delete(c, c->global, f.name, &removed) == FVM_OK && removed);
    }
    if (kind == 12)
    {
        const char *mixed[] = {"nUmBeR", "bOoLeAn", "sTrInG"};
        value = class_value(f.factory);
        assert(fvm_set_global(c, mixed[type], &value) == FVM_OK);
    }
}

static FVM_Result boxing_expected(const BoxingFixture &f)
{
    if (!f.version) return FVM_OK;
    if (f.kind == 8) return FVM_ERROR;
    if (f.kind == 9 || f.kind == 13) return FVM_EXCEPTION;
    return FVM_OK;
}

static bool boxing_custom(const BoxingFixture &f)
{
    return f.version && (f.kind == 5 || f.kind == 6 || f.kind == 11 ||
                         (f.kind == 12 && f.version < 7));
}

static bool boxing_fallback(const BoxingFixture &f)
{
    return f.version && (f.kind == 1 || f.kind == 2 || f.kind == 3 ||
                         f.kind == 4 || f.kind == 7 || f.kind == 10);
}

static void boxing_check(BoxingFixture &f, FVM_Object *object, bool adapter)
{
    assert(object);
    FVM_Value value;
    if (boxing_fallback(f))
    {
        if (adapter) assert(object == f.context->global);
        else
        {
            assert(!object->prototype);
            int own = 0;
            assert(fvm_object_has_own(f.context, object, "__constructor__", &own) == FVM_OK && !own);
        }
    }
    else if (boxing_custom(f))
    {
        assert(fvm_object_get(f.context, object, "boxed", &value) == FVM_OK);
        boxing_equal(value, f.input);
        assert(fvm_object_get(f.context, object, "__constructor__", &value) == FVM_OK && value.object == f.factory);
    }
    else
    {
        const int resources[] = {FVM_RESOURCE_NUMBER, FVM_RESOURCE_BOOLEAN, FVM_RESOURCE_STRING};
        assert(object->resource_kind == resources[f.type] && object->prototype == f.prototype && object->resource_result_valid);
        boxing_equal(object->resource_result, f.input);
    }
}

/* Nested automatic compilation may enter both the outer body and a boxing factory. */
static FVM_Value boxing_run(FVM_Context *c, const Program &program, unsigned mode, FVM_Result expected)
{
    FVM_Object *function = 0;
    FVM_JitCode *native = 0;
    FVM_Value out;
    assert(fvm_function_create_avm1(c, program.bytes.begin(), program.bytes.size(), 0, 0, 0, &function) == FVM_OK &&
           fvm_gc_pin(c, function) == FVM_OK);
    if (fvm_jit_available())
        assert(fvm_jit_compile(program.bytes.begin(), program.bytes.size(), &native) == FVM_OK);
    assert(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    if (mode == 1) assert(fvm_function_set_body(c, function, fvm_jit_body, native, 0) == FVM_OK);
    unsigned long before = c->execution_mode.stats.native_entries;
    FVM_GCRootFrame *roots = c->gc_frames;
    assert(fvm_function_invoke(c, function, 0, 0, 0, &out) == expected);
    assert(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth);
    if (!mode) assert(c->execution_mode.stats.native_entries == before);
    else assert(c->execution_mode.stats.native_entries > before);
    if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
    if (expected == FVM_OK && out.type == FVM_VALUE_OBJECT) assert(fvm_gc_pin(c, out.object) == FVM_OK);
    assert(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
           fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK);
    if (native) assert(fvm_jit_destroy(native) == FVM_OK);
    return out;
}
#endif
