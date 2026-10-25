/* Initializer progress fixtures using the established SWF6-8 loader and public native callbacks. */
#define main initializer_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#include "movieclip_tick_fixture.hpp"
#include "../src/standard/movieclip_constants.h"
#include "flashvm/jit.h"
#include "flashvm/execution.h"


static void initializer(Bytes &tags, const Bytes &code, unsigned id = 7)
{
    Bytes payload;
    u16(payload, id);
    for (size_t i = 0; i < code.size(); ++i)
    {
        payload.push_back(code[i]);
    }
    tag(tags, 59, payload);
}


struct InitProbe
{
    unsigned calls[4];
    FVM_Object *root;
    FVM_Object *library;
    FVM_Object *table;
    unsigned mutation;
    bool reentered;
};


static Bytes init_call(unsigned ordinal)
{
    Bytes code;
    integer(code, ordinal);
    integer(code, 1);
    text(code, "probeInit");
    code.push_back(0x3d);
    code.push_back(0x17);
    code.push_back(0);
    return code;
}


static FVM_Object *init_root(FVM_Context *c, unsigned version, unsigned count)
{
    Bytes controls, tags, empty;
    tag(controls, 0, empty);
    tag(tags, 39, sprite(7, 0, controls));
    for (unsigned i = 0; i < count; ++i)
    {
        initializer(tags, init_call(i));
    }
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 1);
    swf[3] = (unsigned char)version;
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}


static FVM_Value init_hex(FVM_Context *c, const Bytes &code)
{
    const char digits[] = "0123456789abcdef";
    fvm_detail::vector<char> hex;
    for (size_t i = 0; i < code.size(); ++i)
    {
        hex.push_back(digits[code[i] >> 4]);
        hex.push_back(digits[code[i] & 15]);
    }
    FVM_Value v;
    if (!(fvm_value_string_bytes(&v, hex.begin(), hex.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *holder = 0;
    if (!(fvm_object_create(c, &holder) == FVM_OK && fvm_object_set(c, holder, "code", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return holder->properties["code"];
}


static FVM_Result
init_probe(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    InitProbe *p = static_cast<InitProbe *>(raw);
    if (!(receiver && receiver->owner == c && n == 1 && args[0].type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned id = (unsigned)args[0].number;
    if (!(id < 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->calls[id];
    if (id == 0 && p->mutation == 1)
    {
        p->table->indexed[1] = init_hex(c, init_call(3));
    }
    if (id == 0 && p->mutation == 2)
    {
        p->table->indexed[3] = init_hex(c, init_call(3));
    }
    if (id == 0 && p->mutation == 3 && !p->reentered)
    {
        p->reentered = true;
        if (!(fvm_movieclip_advance(c, p->root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static void init_bind(FVM_Context *c, FVM_Object *root, InitProbe &p)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        p.calls[i] = 0;
    }
    p.root = root;
    p.library = root->resource_result.object->properties["library"].object;
    p.table = p.library->properties["initializers"].object;
    p.mutation = 0;
    p.reentered = false;
    if (!(fvm_register_function_rooted(c, "probeInit", init_probe, &p, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void init_clean(FVM_Context *c, const InitProbe &p)
{
    tick_clean(c);
    if (!(!p.library->swf_initializer_running))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
