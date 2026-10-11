#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include <assert.h>
#include <string.h>
/* Independent CRC repair ensures the malformed string reaches graph validation,
 * rather than only exercising the envelope checksum check. */
static void repair(unsigned char *data, size_t size)
{
    unsigned long crc = 0xffffffffUL;
    size_t i;
    unsigned int bit;
    for (i = 16; i < size; ++i)
    {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320UL : 0);
    }
    crc ^= 0xffffffffUL;
    for (bit = 0; bit < 4; ++bit)
        data[12 + bit] = (unsigned char)(crc >> ((3 - bit) * 8));
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *s = 0;
    FVM_Value input, out, text;
    unsigned char *data = 0;
    size_t size = 0, index;
    int found = 0;
    unsigned char checksum[4];
    const char *marker = "tamper-string-payload";
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_string(&input, marker);
    assert(fvm_construct(c, "String", 1, &input, &s) == FVM_OK);
    fvm_value_undefined(&input);
    input.type = FVM_VALUE_OBJECT;
    input.object = s;
    assert(fvm_value_serialize(c, &input, &data, &size) == FVM_OK);
    for (index = 16; index + strlen(marker) <= size; ++index)
        if (!memcmp(data + index, marker, strlen(marker)))
        {
            found = 1;
            break;
        }
    assert(found);
    memcpy(checksum, data + 12, 4);
    repair(data, size);
    assert(!memcmp(checksum, data + 12, 4));
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_OK);
    data[index] = 0xc0;
    repair(data, size);
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT);
    assert(out.type == FVM_VALUE_UNDEFINED);
    data[index] = 0;
    repair(data, size);
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT);
    data[index] = (unsigned char)marker[0];
    repair(data, size);
    assert(fvm_value_deserialize(other, data, size, &out) == FVM_OK);
    assert(fvm_object_call(other, out.object, "valueOf", 0, 0, &text) == FVM_OK &&
           !strcmp(text.string, marker));
    fvm_serialized_free(data);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
