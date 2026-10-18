/* Independent IEEE binary64 byte oracle, ordered E through SQRT2. */
#ifndef FVM_TEST_MATH_CONSTANT_FIXTURE_HPP
#define FVM_TEST_MATH_CONSTANT_FIXTURE_HPP
#include "math_attribute_fixture.hpp"
static const unsigned char math_constant_bytes[8][8] = {
    {0x69, 0x57, 0x14, 0x8b, 0x0a, 0xbf, 0x05, 0x40},
    {0x16, 0x55, 0xb5, 0xbb, 0xb1, 0x6b, 0x02, 0x40},
    {0xef, 0x39, 0xfa, 0xfe, 0x42, 0x2e, 0xe6, 0x3f},
    {0x0e, 0xe5, 0x26, 0x15, 0x7b, 0xcb, 0xdb, 0x3f},
    {0xfe, 0x82, 0x2b, 0x65, 0x47, 0x15, 0xf7, 0x3f},
    {0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40},
    {0xcd, 0x3b, 0x7f, 0x66, 0x9e, 0xa0, 0xe6, 0x3f},
    {0xcd, 0x3b, 0x7f, 0x66, 0x9e, 0xa0, 0xf6, 0x3f}};
static void math_constant_check(unsigned index, double number)
{
    const unsigned one = 1; const bool little = *(const unsigned char *)&one != 0;
    const unsigned char *bytes = (const unsigned char *)&number;
    assert(sizeof(number) == 8);
    for (unsigned i = 0; i < 8; ++i) assert(bytes[little ? i : 7-i] == math_constant_bytes[index][i]);
}
static void math_constant_namespace(FVM_Context *c, FVM_Object *math)
{
    for (unsigned i = 0; i < 8; ++i) { FVM_Value value; assert(fvm_object_get(c, math, math_fields[i], &value) == FVM_OK && value.type == FVM_VALUE_NUMBER); math_constant_check(i, value.number); }
}
#endif
