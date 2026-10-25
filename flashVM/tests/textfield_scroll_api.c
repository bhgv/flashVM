/* Strict C89 viewport ABI admission in FONTS and explicit NO_FONTS profiles. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "fixtures/font_data.h"
#include <string.h>


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *field = 0;
    FVM_TextFieldScrollInfo info, old;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_text_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&info, 77, sizeof(info));
    info.struct_size = sizeof(info);
    old = info;
    if (!(fvm_textfield_scroll_info(context, 0, &info) == FVM_INVALID_ARGUMENT && !memcmp(&info, &old, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_standard_text_register_font(context, "Geometry", fvm_test_font, sizeof(fvm_test_font), 0) == FVM_OK &&
          fvm_textfield_create(context, 0, 100, 100, &field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_textfield_scroll_info(context, field, &info) == FVM_OK && info.scroll == 1 && info.hscroll == 0 &&
          info.maxscroll == 1 && info.maxhscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(fvm_standard_text_register_font(context, "Geometry", fvm_test_font, sizeof(fvm_test_font), 0) ==
          FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_textfield_create(context, 0, 100, 100, &field) == FVM_UNSUPPORTED && !field))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
