#include <stdio.h>
#include <stdlib.h>
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/function.h"
#include "../src/details/vector.hpp"
#include "../src/swf/reader.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static inline void u16(Bytes &b, unsigned int n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}


static inline void u32(Bytes &b, unsigned long n)
{
    for (int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
}


static inline void text(Bytes &b, const char *s)
{
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}


static inline void tag(Bytes &b, unsigned int code, const Bytes &data)
{
    if (data.size() < 63)
    {
        u16(b, (code << 6) | (unsigned int)data.size());
    }
    else
    {
        u16(b, (code << 6) | 63);
        u32(b, (unsigned long)data.size());
    }
    for (size_t i = 0; i < data.size(); ++i)
    {
        b.push_back(data[i]);
    }
}


struct Bits
{
    Bytes b;
    unsigned int bit;


    Bits() : bit(0)
    {
    }


    void put(unsigned long v, unsigned int n)
    {
        for (unsigned int i = n; i; --i)
        {
            if (!bit)
            {
                b.push_back(0);
            }
            b[b.size() - 1] |= (unsigned char)(((v >> (i - 1)) & 1) << (7 - bit));
            bit = (bit + 1) & 7;
        }
    }
};


static inline void append(Bytes &b, const Bytes &a)
{
    for (size_t i = 0; i < a.size(); ++i)
    {
        b.push_back(a[i]);
    }
}


static inline Bytes matrix(long x, long y, bool skew = false)
{
    Bits bits;
    bits.put(0, 1);
    bits.put(skew ? 1 : 0, 1);
    if (skew)
    {
        bits.put(18, 5);
        bits.put(32768, 18);
        bits.put((unsigned long)-65536, 18);
    }
    bits.put(16, 5);
    bits.put((unsigned long)x, 16);
    bits.put((unsigned long)y, 16);
    return bits.b;
}


static inline Bytes color()
{
    Bits bits;
    bits.put(1, 1);
    bits.put(1, 1);
    bits.put(10, 4);
    bits.put(0, 10);
    bits.put(0, 10);
    bits.put(0, 10);
    bits.put(128, 10);
    bits.put(0, 10);
    bits.put(255, 10);
    bits.put(0, 10);
    bits.put(0, 10);
    return bits.b;
}


static inline Bytes
place(unsigned int flags, unsigned int depth, unsigned int id, long x, long y, const char *name = 0, bool skew = false)
{
    Bytes b;
    b.push_back((unsigned char)flags);
    u16(b, depth);
    if (flags & 2)
    {
        u16(b, id);
    }
    if (flags & 4)
    {
        append(b, matrix(x, y, skew));
    }
    if (flags & 8)
    {
        append(b, color());
    }
    if (flags & 16)
    {
        u16(b, 42);
    }
    if (flags & 32)
    {
        text(b, name ? name : "actor");
    }
    return b;
}


static inline Bytes action(unsigned int n)
{
    Bytes b;
    b.push_back(0x96);
    u16(b, 6);
    b.push_back(0);
    text(b, "this");
    b.push_back(0x1c);
    b.push_back(0x96);
    u16(b, 6);
    b.push_back(0);
    text(b, "seen");
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    u32(b, n);
    b.push_back(0x4f);
    b.push_back(0);
    return b;
}


static inline void definition(Bytes &tags)
{
    Bytes b, empty;
    u16(b, 7);
    u16(b, 3);
    for (unsigned int i = 1; i <= 3; ++i)
    {
        tag(b, 12, action(i * 11));
        tag(b, 1, empty);
    }
    tag(b, 0, empty);
    tag(tags, 39, b);
}


static inline Bytes movie(const Bytes &tags, unsigned int frames)
{
    Bytes b;
    b.push_back('F');
    b.push_back('W');
    b.push_back('S');
    b.push_back(8);
    u32(b, (unsigned long)tags.size() + 14);
    b.push_back(8);
    b.push_back(0);
    u16(b, 12 * 256);
    u16(b, frames);
    append(b, tags);
    return b;
}


static inline FVM_Object *load(FVM_Context *c, const Bytes &tags, unsigned int frames)
{
    Bytes b = movie(tags, frames);
    FVM_Object *o = 0;
    FVM_Result r = fvm_movieclip_load_swf(c, b.begin(), b.size(), "root", &o);
    if (r != FVM_OK)
    {
        std::fprintf(stderr, "load: %d\n", (int)r);
    }
    if (!(r == FVM_OK && o))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
}


static inline FVM_Value get(FVM_Context *c, FVM_Object *o, const char *key)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, key, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v;
}


static inline void num(FVM_Context *c, FVM_Object *o, const char *key, double n)
{
    FVM_Value v = get(c, o, key);
    if (v.type != FVM_VALUE_NUMBER || std::fabs(v.number - n) > 1e-9)
    {
        std::fprintf(stderr, "%s = %g wanted %g\n", key, v.number, n);
    }
    if (!(v.type == FVM_VALUE_NUMBER && std::fabs(v.number - n) < 1e-9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void tick(FVM_Context *c, FVM_Object *o)
{
    if (!(fvm_movieclip_advance(c, o, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void seek(FVM_Context *c, FVM_Object *o, unsigned int frame)
{
    FVM_Value v, out;
    fvm_value_number(&v, frame);
    if (!(fvm_object_call(c, o, "gotoAndStop", 1, &v, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(c, o);
}


static inline void pose(FVM_Context *c, FVM_Object *o, double x, double y, bool skew = false)
{
    FVM_Affine m;
    if (!(fvm_movieclip_get_matrix(c, o, &m) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fabs(m.xx - 1) < 1e-9 && std::fabs(m.yy - 1) < 1e-9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fabs(m.yx - (skew ? 0.5 : 0)) < 1e-9 && std::fabs(m.xy - (skew ? -1 : 0)) < 1e-9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(std::fabs(m.x0 - x) < 1e-9 && std::fabs(m.y0 - y) < 1e-9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void readers()
{
    const unsigned char extremes[] = {0x80, 0, 0, 0, 0x7f, 0xff, 0xff, 0xff};


    FVM_SWFReader r(extremes, sizeof(extremes));
    if (!(r.signed_bits(32) == (-2147483647L - 1)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(r.signed_bits(32) == 2147483647L && r.finished()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(r.bits(1) == 0 && !r.good()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    FVM_SWFReader invalid(extremes, 1);
    if (!(invalid.bits(33) == 0 && !invalid.good()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b = matrix(-120, 300, true);


    FVM_SWFReader mr(b.begin(), b.size());
    FVM_Affine m;
    if (!(fvm_swf_read_matrix(mr, m) && mr.finished()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(m.x0 == -6 && m.y0 == 15 && m.xy == -1 && m.yx == 0.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    FVM_SWFReader truncated(b.begin(), b.size() - 1);
    if (!(!fvm_swf_read_matrix(truncated, m)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b = color();


    FVM_SWFReader cr(b.begin(), b.size());
    FVM_SWFColorTransform color_value;
    if (!(fvm_swf_read_color(cr, true, color_value) && cr.finished()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(color_value.multiply[3] == 0.5 && color_value.add[1] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    FVM_SWFReader bad_string(extremes + 4, 4);
    fvm_detail::string s;
    if (!(!bad_string.string(s)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void rectangle(FVM_Context *c, FVM_Object *o)
{
    FVM_Value a[2], out;
    fvm_value_number(a, 0xff0000);
    if (!(fvm_object_call(c, o, "beginFill", 1, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double xy[] = {0, 0, 8, 0, 8, 8, 0, 8, 0, 0};
    for (unsigned int i = 0; i < 10; i += 2)
    {
        fvm_value_number(a, xy[i]);
        fvm_value_number(a + 1, xy[i + 1]);
        if (!(fvm_object_call(c, o, i ? "lineTo" : "moveTo", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_object_call(c, o, "endFill", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline void matrices(FVM_Context *c)
{
    FVM_Object *o = 0;
    if (!(fvm_movieclip_create(c, "matrix", 1, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double components[][4] = {
        {1, 0.5, -1, 1}, {-2, 0, 0.25, 3}, {0, 0, 2, -3}, {0, 0, 0, 0}, {1e-200, 0, 0, 1e-200}, {1e200, 0, 0, 1e200}};
    for (unsigned int i = 0; i < sizeof(components) / sizeof(components[0]); ++i)
    {
        FVM_Affine m, actual;
        m.xx = components[i][0];
        m.yx = components[i][1];
        m.xy = components[i][2];
        m.yy = components[i][3];
        m.x0 = 3.5;
        m.y0 = -7;
        if (!(fvm_movieclip_set_matrix(c, o, &m) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_get_matrix(c, o, &actual) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double scale = std::fabs(m.xx) + std::fabs(m.yx) + std::fabs(m.xy) + std::fabs(m.yy);
        if (!scale)
        {
            scale = 1;
        }
        if (!(std::fabs(actual.xx - m.xx) / scale < 1e-12 && std::fabs(actual.yx - m.yx) / scale < 1e-12))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::fabs(actual.xy - m.xy) / scale < 1e-12 && std::fabs(actual.yy - m.yy) / scale < 1e-12))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(actual.x0 == m.x0 && actual.y0 == m.y0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Affine last, invalid;
    if (!(fvm_movieclip_get_matrix(c, o, &last) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid = last;
    invalid.xx = HUGE_VAL;
    if (!(fvm_movieclip_set_matrix(c, o, &invalid) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_matrix(c, o, &invalid) == FVM_OK && invalid.xx == last.xx))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_matrix(c, 0, &last) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v, out;
    fvm_value_number(&v, 1e200);
    if (!(fvm_object_call(c, o, "beginFill", 1, &v, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, -1e200);
    if (!(fvm_object_call(c, o, "beginFill", 1, &v, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline FVM_Result
constructor(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *, FVM_Value *out, void *userdata)
{
    if (!(argc == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++*(unsigned int *)userdata;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long freed = 99;
    if (!(fvm_gc_step(c, &freed) == FVM_OK && freed == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, self, 1, 2);
    num(c, self, "_alpha", 50);
    FVM_Value p = get(c, self, "_parent");
    if (!(p.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, p.object, "actor").object == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, self, "seen").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}

#ifndef FVM_SCENE_FIXTURE_ONLY


int main()
{
    readers();
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
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
    matrices(c);
    Bytes tags, empty, removal;
    definition(tags);
    tag(tags, 26, place(2 | 4 | 8 | 16 | 32, 1, 7, 20, 40));
    tag(tags, 1, empty);
    tag(tags, 26, place(1 | 4 | 32, 1, 0, 100, -60, "ignoredRename", true));
    tag(tags, 1, empty);
    u16(removal, 1);
    tag(tags, 28, removal);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    FVM_Object *root = load(c, tags, 3);
    tick(c, root);
    FVM_Object *actor = get(c, root, "actor").object;
    if (!(actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, actor) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, actor, 1, 2);
    num(c, actor, "seen", 11);
    num(c, actor, "_alpha", 50);
    rectangle(c, actor);
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface;
    surface.pixels = pixels;
    surface.width = surface.height = 16;
    surface.stride = 64;
    FVM_Result rendered = fvm_movieclip_render(c, root, &surface);
    if (rendered == FVM_OK)
    {
        unsigned char *p = pixels + 4 * (4 * 16 + 4);
        if (!(p[0] == 0 && p[1] >= 254 && p[2] == 0 && p[3] >= 126 && p[3] <= 129))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(rendered == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value depth;
    if (!(fvm_object_call(c, actor, "getDepth", 0, 0, &depth) == FVM_OK && depth.number == -16383))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(c, root);
    if (!(get(c, root, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, root, "ignoredRename").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, actor, 5, -3, true);
    num(c, actor, "seen", 22);
    num(c, actor, "_alpha", 50);
    FVM_Value value, copy;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = root;
    unsigned char *bytes = 0;
    size_t count = 0;
    if (!(fvm_value_serialize(c, &value, &bytes, &count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(other, bytes, count, &copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(bytes);
    FVM_Object *restored = copy.object;
    if (!(fvm_gc_pin(other, restored) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(other, get(other, restored, "actor").object, 5, -3, true);
    tick(other, restored);
    if (!(get(other, restored, "actor").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(other, restored);
    pose(other, get(other, restored, "actor").object, 1, 2);
    tick(c, root);
    if (!(get(c, root, "actor").type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(get(c, actor, "_parent").type != FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(c, root);
    if (!(get(c, root, "actor").object != actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, get(c, root, "actor").object, "seen", 11);
    FVM_Object *jump = load(c, tags, 3);
    seek(c, jump, 2);
    actor = get(c, jump, "actor").object;
    if (!(actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, actor, 5, -3, true);
    num(c, actor, "seen", 11);
    num(c, actor, "_alpha", 50);
    seek(c, jump, 1);
    if (!(get(c, jump, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, actor, 1, 2);
    Bytes one;
    definition(one);
    tag(one, 26, place(2 | 4 | 32, 1, 7, 0, 0));
    tag(one, 1, empty);
    tag(one, 0, empty);
    root = load(c, one, 1);
    tick(c, root);
    actor = get(c, root, "actor").object;
    tick(c, root);
    if (!(get(c, root, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, actor, "seen", 22);
    tick(c, root);
    if (!(get(c, root, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, actor, "seen", 33);
    Bytes replacement;
    definition(replacement);
    tag(replacement, 26, place(2 | 4 | 8 | 32, 1, 7, 20, 40));
    tag(replacement, 1, empty);
    tag(replacement, 26, place(1 | 2, 1, 7, 0, 0));
    tag(replacement, 1, empty);
    tag(replacement, 0, empty);
    root = load(c, replacement, 2);
    tick(c, root);
    actor = get(c, root, "actor").object;
    if (!(fvm_gc_pin(c, actor) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(c, root);
    FVM_Object *next = get(c, root, "actor").object;
    if (!(next && next == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, next, 1, 2);
    num(c, next, "_alpha", 50);
    num(c, next, "seen", 22);
    tick(c, root);
    if (!(get(c, root, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, actor, "seen", 33);
    FVM_Channel *ch = 0;
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value.object = root;
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
    tick(other, copy.object);
    pose(other, get(other, copy.object, "actor").object, 1, 2);
    fvm_channel_destroy(ch);
    FVM_Channel *file = 0;
    FVM_Result opened = fvm_channel_open_file("flashvm-scene-test.bin", "w", &file);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, file, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(file);
        if (!(fvm_channel_open_file("flashvm-scene-test.bin", "r", &file) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, file, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(file);
        if (!(std::remove("flashvm-scene-test.bin") == 0))
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
        tick(other, copy.object);
        pose(other, get(other, copy.object, "actor").object, 1, 2);
    }
    else
    {
        if (!(opened == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes nested;
    definition(nested);
    Bytes outer;
    u16(outer, 8);
    u16(outer, 1);
    tag(outer, 26, place(2 | 4 | 32, 1, 7, 20, 40, "inner", true));
    tag(outer, 1, empty);
    tag(outer, 0, empty);
    tag(nested, 39, outer);
    tag(nested, 26, place(2 | 4 | 8 | 32, 1, 8, 40, 60, "outer"));
    tag(nested, 1, empty);
    tag(nested, 0, empty);
    root = load(c, nested, 1);
    tick(c, root);
    FVM_Object *container = get(c, root, "outer").object, *inner = get(c, container, "inner").object;
    if (!(inner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, inner, 1, 2, true);
    FVM_Object *point = 0;
    if (!(fvm_object_create(c, &point) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 2);
    if (!(fvm_object_set(c, point, "x", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 1);
    if (!(fvm_object_set(c, point, "y", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value.type = FVM_VALUE_OBJECT;
    value.object = point;
    if (!(fvm_object_call(c, inner, "localToGlobal", 1, &value, &copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, point, "x", 4);
    num(c, point, "y", 7);
    tick(c, root);
    if (!(get(c, container, "inner").object == inner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    num(c, inner, "seen", 22);
    Bytes legacy_tags;
    definition(legacy_tags);
    Bytes legacy_place;
    u16(legacy_place, 7);
    u16(legacy_place, 1);
    append(legacy_place, matrix(60, 80));
    tag(legacy_tags, 4, legacy_place);
    tag(legacy_tags, 1, empty);
    Bytes legacy_remove;
    u16(legacy_remove, 7);
    u16(legacy_remove, 1);
    tag(legacy_tags, 5, legacy_remove);
    tag(legacy_tags, 1, empty);
    tag(legacy_tags, 0, empty);
    root = load(c, legacy_tags, 2);
    tick(c, root);
    fvm_value_number(&value, -16383);
    if (!(fvm_object_call(c, root, "getInstanceAtDepth", 1, &value, &copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(copy.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pose(c, copy.object, 3, 4);
    tick(c, root);
    if (!(fvm_object_call(c, root, "getInstanceAtDepth", 1, &value, &copy) == FVM_OK &&
          copy.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned int k = 0; k < 5; ++k)
    {
        Bytes bad;
        definition(bad);
        Bytes p = place(2 | 4 | 32, k == 0 ? 0 : 1, k == 1 ? 8 : 7, 20, 40);
        if (k == 2)
        {
            p.resize(p.size() - 1);
        }
        if (k == 3)
        {
            p.push_back(0);
        }
        if (k == 4)
        {
            p[0] |= 0x40;
        }
        tag(bad, 26, p);
        tag(bad, 1, empty);
        tag(bad, 0, empty);
        Bytes swf = movie(bad, 1);
        FVM_Object *out = root;
        FVM_Result r = fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &out);
        if (!(r == (k == 4 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT) && !out))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes registered;
    definition(registered);
    Bytes exports;
    u16(exports, 1);
    u16(exports, 7);
    text(exports, "Hero");
    tag(registered, 56, exports);
    tag(registered, 26, place(2 | 4 | 8 | 32, 1, 7, 20, 40));
    tag(registered, 1, empty);
    tag(registered, 1, empty);
    tag(registered, 0, empty);
    unsigned int calls = 0;
    FVM_Object *function = 0;
    if (!(fvm_function_create_native(c, constructor, &calls, 0, &function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value klass, movie_class;
    if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "MovieClip", &movie_class) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value prototype = get(c, movie_class.object, "prototype");
    if (!(fvm_object_set(c, function, "prototype", &prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value registration[2], result;
    fvm_value_string(registration, "Hero");
    fvm_value_undefined(registration + 1);
    registration[1].type = FVM_VALUE_OBJECT;
    registration[1].object = function;
    if (!(fvm_object_call(c, klass.object, "registerClass", 2, registration, &result) == FVM_OK && result.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    root = load(c, registered, 2);
    tick(c, root);
    if (!(calls == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    actor = get(c, root, "actor").object;
    tick(c, root);
    tick(c, root);
    if (!(calls == 1 && get(c, root, "actor").object == actor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}

#endif
