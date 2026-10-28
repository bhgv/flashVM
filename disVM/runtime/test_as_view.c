/* Execute the actual source-compiled IDE AS view in FlashVM, independently of Dis. */
#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "ide_view_swf.h"
#include <stdio.h>
#include <string.h>

/* Borrow context and verify one AS-authored numeric property, without coercion. */
static int number(FVM_Context *context, FVM_Object *object, const char *key, double expected)
{
    FVM_Value value;
    return fvm_object_get(context, object, key, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
           value.number == expected;
}

/* Run the shipped AS actions with independent bindings, GC and changing geometry.
 * No arguments. Return zero only for correct dynamic strings/properties on every frame. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *panel = 0;
    FVM_Value value;
    int result = 1;
    int frame;
    const char text[] = "FILE  \xce\xb1\xf0\x9f\x98\x80.b\nDIR   source\n";
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK ||
        fvm_context_create(runtime, &context) != FVM_OK || fvm_object_create(context, &panel) != FVM_OK)
    {
        goto done;
    }
    if (fvm_load_memory(context, ide_view_swf, sizeof(ide_view_swf)) != FVM_OK)
    {
        goto done;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = panel;
    if (fvm_set_global(context, "idePanel", &value) != FVM_OK)
    {
        goto done;
    }
    for (frame = 0; frame < 8; frame++)
    {
        if (fvm_value_string_bytes(&value, text, sizeof(text) - 1) != FVM_OK ||
            fvm_set_global(context, "idePanelText", &value) != FVM_OK)
        {
            goto done;
        }
        fvm_value_number(&value, 320 + frame * 20);
        if (fvm_set_global(context, "ideWidth", &value) != FVM_OK)
        {
            goto done;
        }
        fvm_value_number(&value, 400);
        if (fvm_set_global(context, "ideHeight", &value) != FVM_OK)
        {
            goto done;
        }
        fvm_value_number(&value, 100 + frame);
        if (fvm_set_global(context, "idePanelHeight", &value) != FVM_OK)
        {
            goto done;
        }
        /* [GC:] The global binding owns panel; borrowed native address survives collection. */
        if (fvm_gc_collect(context, 0) != FVM_OK || fvm_swf_execute_actions(context) != FVM_OK ||
            !number(context, panel, "_width", 304 + frame * 20) || !number(context, panel, "_height", 92 + frame) ||
            !number(context, panel, "_x", 8) || !number(context, panel, "_y", 306 - frame) ||
            fvm_object_get(context, panel, "text", &value) != FVM_OK || value.type != FVM_VALUE_STRING ||
            fvm_value_string_size(&value) != sizeof(text) - 1 || memcmp(value.string, text, sizeof(text) - 1))
        {
            goto done;
        }
        if (fvm_object_get(context, panel, "multiline", &value) != FVM_OK || value.type != FVM_VALUE_BOOLEAN ||
            !value.boolean)
        {
            goto done;
        }
    }
    result = 0;
done:
    if (context)
    {
        fvm_context_destroy(context);
    }
    if (runtime)
    {
        fvm_runtime_destroy(runtime);
    }
    if (result)
    {
        fprintf(stderr, "compiled AS view did not produce the required FlashVM properties\n");
    }
    else
    {
        puts("PASS compiled AS view: actual AVM1, Unicode strings, geometry and rooted owner across GC");
    }
    return result;
}
