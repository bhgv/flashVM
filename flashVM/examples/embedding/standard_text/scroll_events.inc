/* Observe actual field-first scroll notifications with host-owned userdata alive through teardown. */
#define FVM_EXAMPLE_SCROLL_EVENT_TRACE 16U /* Bound the caller-owned tutorial trace without allocation. */
typedef struct FVM_ExampleScrollEvents { /* Keep this host state alive until context destruction. */
    FVM_Object *field; /* Borrow the source field while the context owns its graph. */
    unsigned int trace[FVM_EXAMPLE_SCROLL_EVENT_TRACE]; /* Record own handler as one and listener as two. */
    unsigned int count; /* Number of synchronous callbacks already recorded. */
} FVM_ExampleScrollEvents; /* This is host userdata, not a serialized VM structure. */
#ifdef FVM_HAS_FONTS /* Real fields require the bundled font implementation. */
static FVM_Result scroll_event_callback(FVM_Context *context, FVM_Object *self, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* Run under the existing native accessor barrier. */
{ /* Validate callback ABI and borrowed source identity before recording anything. */
    FVM_ExampleScrollEvents *events = (FVM_ExampleScrollEvents *)raw; /* Borrow the main function's stable host state. */
    FVM_TextFieldScrollInfo info; /* Observe committed viewport geometry without another script callback. */
    if (!events || count != 1 || args[0].type != FVM_VALUE_OBJECT || args[0].object != events->field || events->count >= FVM_EXAMPLE_SCROLL_EVENT_TRACE) return FVM_INVALID_ARGUMENT; /* Reject malformed or excessive delivery. */
    info.struct_size = sizeof(info); /* Supply the exact native observation ABI. */
    if (fvm_textfield_scroll_info(context, events->field, &info) != FVM_OK) return FVM_ERROR; /* The source position has already been committed. */
    events->trace[events->count++] = self == events->field ? 1U : 2U; /* Preserve actual receiver order rather than requested registration order alone. */
    fvm_value_undefined(out); return FVM_OK; /* No owned or managed output escapes this callback. */
} /* The caller retains userdata through all field/listener lifetimes. */
static int scroll_events_demo(FVM_Context *context, FVM_Object *field, FVM_Object *format, FVM_ExampleScrollEvents *events) /* Borrow a field already prepared by scroll_demo. */
{ /* Keep declarations before statements for strict C89. */
    FVM_Object *listener = 0; FVM_Value argument, result, value, replacement[3]; /* Own only managed graph references in this context. */
    FVM_TextFieldScrollInfo info; unsigned int index; /* Copy final scalar state and iterate the bounded trace. */
    events->field = field; events->count = 0; /* Initialize host userdata before registering any callback. */
    if (fvm_object_define_method_rooted(context, field, "onScroller", scroll_event_callback, events, field) != FVM_OK) return 1; /* Capture the managed field while userdata stays host-owned. */
    if (fvm_object_create(context, &listener) != FVM_OK) return 1; /* Create a listener with independent receiver identity. */
    if (fvm_object_define_method_rooted(context, listener, "onScroller", scroll_event_callback, events, field) != FVM_OK) return 1; /* Retain the source graph from this listener callback. */
    fvm_value_undefined(&argument); argument.type = FVM_VALUE_OBJECT; argument.object = listener; /* Borrow the listener through synchronous registration. */
    if (fvm_object_call(context, field, "addListener", 1, &argument, &result) != FVM_OK || result.type != FVM_VALUE_BOOLEAN || !result.boolean) return 1; /* Registration owns a strong ordinary managed edge. */
    fvm_value_number(&value, 1); /* Move from the prior scroll_demo top line two to line one. */
    if (fvm_object_set(context, field, "scroll", &value) != FVM_OK || fvm_object_set(context, field, "scroll", &value) != FVM_OK) return 1; /* The second unchanged write must emit nothing. */
    fvm_value_number(&value, 3); /* Change horizontal position from seven pixels to three. */
    if (fvm_object_set(context, field, "hscroll", &value) != FVM_OK) return 1; /* Deliver own handler, then the registered listener. */
    if (fvm_object_call(context, field, "removeListener", 1, &argument, &result) != FVM_OK || !result.boolean) return 1; /* The next event must exclude this identity. */
    fvm_value_number(&value, 2); /* Return to the second visual line after listener removal. */
    if (fvm_object_set(context, field, "scroll", &value) != FVM_OK) return 1; /* Only the field handler remains. */
    fvm_value_number(&value, 60); /* Widen beyond the logical text width and remove horizontal overflow. */
    if (fvm_object_set(context, field, "_width", &value) != FVM_OK) return 1; /* Range change delivers one field callback after listener removal. */
    fvm_value_number(&value, 200); /* Fit every visual line and clamp the top line back to one. */
    if (fvm_object_set(context, field, "_height", &value) != FVM_OK) return 1; /* The handler sees the new height and vertical range together. */
    if (fvm_value_string(&value, "AVAVAV") != FVM_OK || fvm_object_set(context, field, "text", &value) != FVM_OK) return 1; /* A longer counted text creates horizontal overflow and one notification. */
    fvm_value_number(replacement, 0); fvm_value_number(replacement + 1, 6); /* Select all six current UTF-16 units with valid replacement bounds. */
    if (fvm_value_string(replacement + 2, "A") != FVM_OK || fvm_object_call(context, field, "replaceText", 3, replacement, &result) != FVM_OK) return 1; /* Publish the owned shorter text before the range callback. */
    fvm_value_number(&value, 160); /* Enlarge the already rooted reusable TextFormat without changing the field yet. */
    if (fvm_object_set(context, format, "size", &value) != FVM_OK) return 1; /* Format-object edits alone do not notify field observers. */
    fvm_value_undefined(&argument); argument.type = FVM_VALUE_OBJECT; argument.object = format; /* Borrow the caller-rooted format during synchronous application. */
    if (fvm_object_call(context, field, "setTextFormat", 1, &argument, &result) != FVM_OK) return 1; /* New current metrics create overflow and notify after publication. */
    info.struct_size = sizeof(info); /* Copy final committed state before printing. */
    if (fvm_textfield_scroll_info(context, field, &info) != FVM_OK) return 1; /* This observation emits no notification. */
    printf("scroll events:"); for (index = 0; index < events->count; ++index) printf(" %u", events->trace[index]); putchar('\n'); /* Show actual synchronous receiver order. */
    printf("scroll events state: top=%lu horizontal=%lu max=%lu/%lu\n", info.scroll, info.hscroll, info.maxscroll, info.maxhscroll); /* Distinguish final line and pixel units. */
    return 0; /* Main keeps events alive until destroying the context and its callbacks. */
} /* No raw userdata is retained beyond its main-function owner. */
#endif /* NO_FONTS retains explicit unavailable tutorial behavior. */
