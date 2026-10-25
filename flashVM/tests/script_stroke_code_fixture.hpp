#ifndef FVM_SCRIPT_STROKE_CODE_FIXTURE_HPP
#define FVM_SCRIPT_STROKE_CODE_FIXTURE_HPP
#include "script_stroke_fixture.hpp"


static void ss_push_text(Bytes &b, const char *s)
{
    b.push_back(0x96);
    u16(b, (unsigned)std::strlen(s) + 2);
    b.push_back(0);
    text(b, s);
}


static void ss_push_number(Bytes &b, unsigned v)
{
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    u32(b, v);
}


static void ss_target(Bytes &b, const char *method, unsigned argc)
{
    ss_push_number(b, argc);
    ss_push_text(b, "paintClip");
    b.push_back(0x1c);
    ss_push_text(b, method);
    b.push_back(0x52);
    b.push_back(0x17);
}


static Bytes ss_code(unsigned cap, unsigned join)
{
    Bytes b;
    ss_push_text(b, join ? "bevel" : "round");
    ss_push_text(b, cap == 1 ? "none" : cap == 2 ? "square" : "round");
    ss_push_text(b, "normal");
    ss_push_number(b, 0);
    ss_push_number(b, 100);
    ss_push_number(b, 0xff0000);
    ss_push_number(b, 4);
    ss_target(b, "lineStyle", 7);
    ss_push_number(b, 8);
    ss_push_number(b, 2);
    ss_target(b, "moveTo", 2);
    ss_push_number(b, 8);
    ss_push_number(b, 14);
    ss_target(b, "lineTo", 2);
    b.push_back(0);
    return b;
}
#endif
