#include <stdio.h>
#include <stdlib.h>
#include "flashvm/function.h"
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/details/vector.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void str(Bytes &b, const char *s)
{
    size_t n = std::strlen(s) + 2;
    b.push_back(0x96);
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
    b.push_back(0);
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}


static void object_value(FVM_Value *v, FVM_Object *object)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = object;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *env = 0, *fn = 0;
    FVM_Channel *ch = 0;
    FVM_Value value, args[2], out, copy, klass, method, again;
    int matches = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &env) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 7);
    if (!(fvm_object_set(c, env, "k", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes code;
    str(code, "n");
    code.push_back(0x1c);
    str(code, "k");
    code.push_back(0x1c);
    code.push_back(0x0a);
    code.push_back(0x3e);
    code.push_back(0);
    const char *parameters[] = {"n"};
    if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 1, parameters, env, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 2);
    if (!(fvm_function_invoke(c, fn, 0, 1, &value, &out) == FVM_OK && out.number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&args[0]);
    args[0].type = FVM_VALUE_NULL;
    args[1] = value;
    if (!(fvm_object_call(c, fn, "call", 2, args, &out) == FVM_OK && out.number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, fn, "call", &method) == FVM_OK && method.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, fn, "call", &again) == FVM_OK && again.object == method.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = fn;
    if (!(fvm_get_global(c, "Function", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(c, &value, &klass, &matches) == FVM_OK && matches))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, fn, "self", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* An undefined captured binding must not fall back to a global value. */
    FVM_Object *shadow = 0;
    fvm_value_number(&args[0], 99);
    if (!(fvm_set_global(c, "missing", &args[0]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&args[0]);
    if (!(fvm_object_set(c, env, "missing", &args[0]) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes shadow_code;
    str(shadow_code, "missing");
    shadow_code.push_back(0x1c);
    shadow_code.push_back(0x3e);
    shadow_code.push_back(0);
    if (!(fvm_function_create_avm1(c, shadow_code.begin(), shadow_code.size(), 0, 0, env, &shadow) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, shadow, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* apply boxes its primitive receiver exactly as call does. */
    FVM_Object *identity = 0;
    Bytes this_code;
    str(this_code, "this");
    this_code.push_back(0x1c);
    this_code.push_back(0x3e);
    this_code.push_back(0);
    if (!(fvm_function_create_avm1(c, this_code.begin(), this_code.size(), 0, 0, 0, &identity) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 17);
    fvm_value_undefined(&args[1]);
    args[1].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(c, identity, "apply", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, out.object, "valueOf", 0, 0, &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *argument_fn = 0;
    Bytes argument_code;
    str(argument_code, "arguments");
    argument_code.push_back(0x1c);
    argument_code.push_back(0x3e);
    argument_code.push_back(0);
    if (!(fvm_function_create_avm1(c, argument_code.begin(), argument_code.size(), 0, 0, 0, &argument_fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&args[0], 5);
    fvm_value_number(&args[1], 6);
    if (!(fvm_function_invoke(c, argument_fn, 0, 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *arguments_object = out.object;
    if (!(fvm_object_get(c, arguments_object, "length", &out) == FVM_OK && out.number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, arguments_object, "1", &out) == FVM_OK && out.number == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, arguments_object, "callee", &out) == FVM_OK && out.object == argument_fn))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Script getters/setters keep their receiver and survive serialization. */
    FVM_Object *get = 0, *set = 0, *property_object = 0;
    Bytes get_code, set_code;
    str(get_code, "this");
    get_code.push_back(0x1c);
    str(get_code, "stored");
    get_code.push_back(0x4e);
    get_code.push_back(0x3e);
    get_code.push_back(0);
    str(set_code, "this");
    set_code.push_back(0x1c);
    str(set_code, "stored");
    str(set_code, "v");
    set_code.push_back(0x1c);
    set_code.push_back(0x4f);
    set_code.push_back(0);
    const char *setter_parameters[] = {"v"};
    if (!(fvm_function_create_avm1(c, get_code.begin(), get_code.size(), 0, 0, env, &get) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, set_code.begin(), set_code.size(), 1, setter_parameters, env, &set) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &property_object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value descriptor_args[3];
    fvm_value_string(descriptor_args, "answer");
    object_value(descriptor_args + 1, get);
    object_value(descriptor_args + 2, set);
    if (!(fvm_object_call(c, property_object, "addProperty", 3, descriptor_args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 81);
    if (!(fvm_object_set(c, property_object, "answer", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, property_object, "answer", &out) == FVM_OK && out.number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Canonical numeric properties use the same getter/setter bridge as names. */
    fvm_value_string(descriptor_args, "0");
    if (!(fvm_object_call(c, property_object, "addProperty", 3, descriptor_args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 81);
    if (!(fvm_object_set(c, property_object, "0", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, property_object, "0", &out) == FVM_OK && out.number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 0;
    if (!(fvm_object_has_own(c, property_object, "0", &own) == FVM_OK && own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, property_object, "hasOwnProperty", 1, descriptor_args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, property_object, "isPropertyEnumerable", 1, descriptor_args, &out) == FVM_OK &&
          out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&out, property_object);
    if (!(fvm_object_set(c, fn, "accessor", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A one-element literal may contain any number, including a negative one. */
    const unsigned char literal[] = {0x96, 5, 0, 7, 253, 255, 255, 255, 0x96, 5, 0, 7, 1, 0, 0, 0, 0x42, 0};
    Bytes literal_code;
    str(literal_code, "literal");
    for (size_t i = 0; i + 1 < sizeof(literal); ++i)
    {
        literal_code.push_back(literal[i]);
    }
    literal_code.push_back(0x1d);
    literal_code.push_back(0);
    if (!(fvm_avm1_execute(c, literal_code.begin(), literal_code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "literal", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, out.object, "0", &out) == FVM_OK && out.number == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 5);
    if (!(fvm_function_invoke(other, copy.object, 0, 1, &value, &out) == FVM_OK && out.number == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, copy.object, "self", &out) == FVM_OK && out.object == copy.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, copy.object, "accessor", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    property_object = out.object;
    if (!(fvm_object_get(other, property_object, "answer", &out) == FVM_OK && out.number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 82);
    if (!(fvm_object_set(other, property_object, "answer", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, property_object, "answer", &out) == FVM_OK && out.number == 82))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Portable call/apply boxing uses captured classes, without receiver globals. */
    object_value(&value, identity);
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 17);
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(other, copy.object, "apply", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(other, out.object, "valueOf", 0, 0, &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "portable this");
    if (!(fvm_object_call(other, copy.object, "call", 1, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(other, out.object, "valueOf", 0, 0, &out) == FVM_OK &&
          !std::strcmp(out.string, "portable this")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "Math", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, klass.object, "abs", &method) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &method, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, -3);
    if (!(fvm_function_invoke(other, copy.object, 0, 1, &value, &out) == FVM_OK && out.number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(ch);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
