/* Capturing effective geometry cannot mutate pending source scroll requests on lazy fields. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_fixture.hpp"
#include "../src/standard/text_state.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    scroll_number(c, field, "scroll", 2);
    scroll_number(c, field, "hscroll", 7);
    scroll_number(c, field, "_width", 100);
    scroll_number(c, field, "_height", 200);
    FVM_TextFieldState state;
    if (!(fvm_textfield_snapshot(field, state) == FVM_OK && state.scroll == 1 && state.hscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value root;
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = field;
    unsigned char *wire = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(c, &root, &wire, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    scroll_number(c, field, "_width", 40);
    scroll_number(c, field, "_height", 94);
    FVM_TextFieldScrollInfo i = scroll_info(c, field);
    if (!(i.scroll == 2 && i.hscroll == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_width", 100);
    scroll_number(c, field, "_height", 200);
    i = scroll_info(c, field);
    if (!(i.scroll == 1 && i.hscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_width", 40);
    scroll_number(c, field, "_height", 94);
    i = scroll_info(c, field);
    if (!(i.scroll == 1 && i.hscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
