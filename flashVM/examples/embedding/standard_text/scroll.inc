/* Demonstrate bounded viewport geometry and independent, translated native contours. */
#ifdef FVM_HAS_FONTS /* Compile the executable scroll demonstration only with real font support. */
static int scroll_demo(FVM_Context *context, FVM_Object *field, FVM_Object *format) /* Borrow managed objects on their owner thread. */
{ /* Keep all C89 declarations before fallible work. */
    FVM_Value value, argument, result; /* Reuse synchronous property and method arguments. */
    FVM_TextFieldScrollInfo info; /* Copy scalar geometry without retaining managed pointers. */
    FVM_Path *before = 0, *after = 0; /* Own each exported contour independently of the VM. */
    FVM_PathBounds first, second; /* Observe the actual translated contour bounds. */
    int status = 1; /* Preserve failure until every query and export succeeds. */
    fvm_value_undefined(&argument); argument.type = FVM_VALUE_OBJECT; argument.object = format; /* Borrow the existing 40px TextFormat. */
    if (fvm_object_call(context, field, "setNewTextFormat", 1, &argument, &result) != FVM_OK) goto cleanup; /* New text adopts the documented analytic face and size. */
    fvm_value_boolean(&value, 0); /* Disable wrapping so explicit hard lines determine this example. */
    if (fvm_object_set(context, field, "wordWrap", &value) != FVM_OK) goto cleanup; /* Keep line starts independent of field width. */
    fvm_value_number(&value, 40); /* Set a 36px inner horizontal viewport after the gutter. */
    if (fvm_object_set(context, field, "_width", &value) != FVM_OK) goto cleanup; /* Preserve ordinary dimension validation. */
    fvm_value_number(&value, 94); /* Fit two 44px bodies separated by the 46px baseline step. */
    if (fvm_object_set(context, field, "_height", &value) != FVM_OK) goto cleanup; /* Keep the outer gutter explicit. */
    if (fvm_value_string(&value, "AV\nAV\nAV") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) goto cleanup; /* Install three uniform authored lines. */
    if (fvm_textfield_path(context, field, &before) != FVM_OK || fvm_path_bounds(before, &first) != FVM_OK) goto cleanup; /* Capture unscrolled geometry before changing offsets. */
    fvm_value_number(&value, 2); /* Select the second visual line as the top. */
    if (fvm_object_set(context, field, "scroll", &value) != FVM_OK) goto cleanup; /* Vertical positions count one-based visual lines. */
    fvm_value_number(&value, 7); /* Shift text seven pixels left inside the viewport. */
    if (fvm_object_set(context, field, "hscroll", &value) != FVM_OK) goto cleanup; /* Horizontal positions count zero-based pixels. */
    info.struct_size = sizeof(info); /* Require the exact observation ABI. */
    if (fvm_textfield_scroll_info(context, field, &info) != FVM_OK) goto cleanup; /* Measure and normalize active offsets without a script callback. */
    if (fvm_textfield_path(context, field, &after) != FVM_OK || fvm_path_bounds(after, &second) != FVM_OK) goto cleanup; /* Export independently owned shifted glyph contours. */
    printf("scroll: top=%lu bottom=%lu max=%lu horizontal=%lu maxHorizontal=%lu\n", info.scroll, info.bottom_scroll, info.maxscroll, info.hscroll, info.maxhscroll); /* Distinguish line and pixel units. */
    printf("scroll viewport: x=%.0f y=%.0f width=%.0f height=%.0f\n", info.clip_x, info.clip_y, info.clip_width, info.clip_height); /* The host clips these field-local coordinates when drawing. */
    printf("scroll path: dx=%.0f dy=%.0f\n", second.x_min-first.x_min, second.y_min-first.y_min); /* Verify actual contour movement rather than echoing requested offsets. */
    status = 0; /* All properties, observations and owned exports succeeded. */
cleanup: /* Destroy both independent paths after success or any failure. */
    fvm_path_destroy(after); fvm_path_destroy(before); /* Null paths are accepted; no managed ownership is transferred. */
    if (status) fputs("TextField scroll demonstration failed\n", stderr); /* Report a normal executable failure without assertions. */
    return status; /* Let the caller release its VM owners. */
} /* End the owner-thread viewport demonstration. */
#endif /* No-font builds retain their explicit unavailable message in main. */
