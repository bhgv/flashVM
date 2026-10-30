/* Colour contours partition one shaped field; source and layout remain authoritative. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"


/* Report a contract failure without compiling out checks in Release. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL range contours: %s\n", message);
        exit(EXIT_FAILURE);
    }
}


/* Exercise actual field shaping, disjoint exports, malformed boundaries and lifetime. */
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    require(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
                fvm_context_create(runtime, &context) == FVM_OK,
            "context");
    FVM_Object *field = counted_field(context);
    FVM_Value value;
    require(fvm_value_string(&value, "AVAV") == FVM_OK && fvm_object_set(context, field, "text", &value) == FVM_OK,
            "source");
    double width = counted_metric(context, field, "textWidth");
    FVM_Path *all = 0, *left = 0, *right = 0, *empty = 0;
    FVM_TextPathRange a[] = {{0, 1}, {2, 3}};
    FVM_TextPathRange b[] = {{1, 2}, {3, 4}};
    require(fvm_textfield_path(context, field, &all) == FVM_OK &&
                fvm_textfield_path_ranges(context, field, a, 2, &left) == FVM_OK &&
                fvm_textfield_path_ranges(context, field, b, 2, &right) == FVM_OK &&
                fvm_textfield_path_ranges(context, field, 0, 0, &empty) == FVM_OK,
            "partition exports");
    require(fvm_path_count(left) > 0 && fvm_path_count(right) > 0 &&
                fvm_path_count(all) == fvm_path_count(left) + fvm_path_count(right) && !fvm_path_count(empty),
            "complete glyph partition");
    require(counted_metric(context, field, "textWidth") == width, "layout unchanged");
    FVM_Path *bad = all;
    FVM_TextPathRange overlap[] = {{0, 3}, {2, 4}};
    require(fvm_textfield_path_ranges(context, field, overlap, 2, &bad) == FVM_INVALID_ARGUMENT && bad == 0,
            "overlap clears output");
    require(fvm_textfield_path_ranges(context, field, 0, 1, &bad) == FVM_INVALID_ARGUMENT && !bad, "null descriptor");
    require(fvm_textfield_path_ranges(context, field, a, FVM_TEXT_PATH_RANGE_LIMIT + 1, &bad) == FVM_INVALID_ARGUMENT &&
                !bad,
            "count rejected before dereference");
    FVM_TextPathRange reversed = {3, 2};
    require(fvm_textfield_path_ranges(context, field, &reversed, 1, &bad) == FVM_INVALID_ARGUMENT, "reversed range");
    require(fvm_value_string(&value, "A\317\200V") == FVM_OK &&
                fvm_object_set(context, field, "text", &value) == FVM_OK,
            "Unicode source");
    FVM_TextPathRange split = {1, 2};
    require(fvm_textfield_path_ranges(context, field, &split, 1, &bad) == FVM_INVALID_ARGUMENT && !bad,
            "UTF-8 continuation endpoint");
    split.first = 2;
    split.last = 3;
    require(fvm_textfield_path_ranges(context, field, &split, 1, &bad) == FVM_INVALID_ARGUMENT,
            "UTF-8 continuation start");
    split.first = 0;
    split.last = 5;
    require(fvm_textfield_path_ranges(context, field, &split, 1, &bad) == FVM_INVALID_ARGUMENT, "counted extent bound");
    require(fvm_value_string(&value, "") == FVM_OK && fvm_object_set(context, field, "text", &value) == FVM_OK,
            "replace source");
    require(fvm_path_count(left) + fvm_path_count(right) == fvm_path_count(all),
            "detached paths survive source replacement");
    fvm_path_destroy(all);
    fvm_path_destroy(left);
    fvm_path_destroy(right);
    fvm_path_destroy(empty);
    fvm_gc_unpin(context, field);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    puts("PASS range contours: whole-layout partition, UTF-8 bounds and detached ownership");
#endif
    return 0;
}
