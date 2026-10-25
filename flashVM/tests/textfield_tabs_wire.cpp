/* Independent wire-size oracle includes tagged number metadata; oversize table counts reject early. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_tabs_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c);
    double stops[64];
    for (unsigned i = 0; i < 64; ++i)
    {
        stops[i] = 40 * (i + 1);
    }
    FVM_Object *format = tabs_format(c, 0, 0);
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "AV");
    FVM_Value root, out;
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    unsigned char *before = 0, *wire = 0;
    size_t old_size = 0, size = 0;
    size_t old_charge = fvm_textfield_snapshot_size(field);
    if (!(fvm_value_serialize(c, &root, &before, &old_size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(before);
    format = tabs_format(c, stops, 64);
    inset_apply(c, field, format, "setTextFormat");
    inset_apply(c, field, format, "setNewTextFormat");
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK && size - old_size == 1672 &&
          fvm_textfield_snapshot_size(field) - old_charge == 1672))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char pattern[] = {0, 0, 0, 64, FVM_VALUE_NUMBER, 0x40, 0x44, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255};
    size_t position = size, matches = 0;
    for (size_t i = 0; i + sizeof(pattern) <= size; ++i)
    {
        if (!std::memcmp(wire + i, pattern, sizeof(pattern)))
        {
            position = i;
            ++matches;
        }
    }
    if (!(matches == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *d = 0;
    if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_install(d) == FVM_OK &&
          fvm_standard_text_install(d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result decoded = fvm_value_deserialize(d, wire, size, &out);
    if (!(decoded == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(d);
    wire[position + 3] = 65;
    if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_install(d) == FVM_OK &&
          fvm_standard_text_install(d) == FVM_OK && fvm_value_deserialize(d, wire, size, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(d);
    wire[position + 3] = 64;
    if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_install(d) == FVM_OK &&
          fvm_standard_text_install(d) == FVM_OK &&
          fvm_value_deserialize(d, wire, position + 14, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(d);
    fvm_serialized_free(wire);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
