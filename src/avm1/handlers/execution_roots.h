/* FlashVM: semantic layouts for common execution-service GC roots and callback arguments. */
#ifndef FVM_EXECUTION_ROOTS_H
#define FVM_EXECUTION_ROOTS_H /* Private shared execution-root layout guard. */
/* [GC:] Original target value during string conversion. */
#define FVM_SELECT_TARGET_INPUT_ROOT 0U
/* [GC:] Converted counted path during display lookup. */
#define FVM_SELECT_TARGET_PATH_ROOT 1U
/* [GC:] Number of retained values in the select_target service layout. */
#define FVM_SELECT_TARGET_ROOT_COUNT 2U
/* [GC:] Left operand transformed by primitive coercion. */
#define FVM_COMPARE_LEFT_ROOT 0U
/* [GC:] Right operand transformed by primitive coercion. */
#define FVM_COMPARE_RIGHT_ROOT 1U
/* [GC:] Left string during dynamic concatenation. */
#define FVM_COMPARE_LEFT_TEXT_ROOT 2U
/* [GC:] Right string during dynamic concatenation. */
#define FVM_COMPARE_RIGHT_TEXT_ROOT 3U
/* [GC:] Number of retained values in the compare service layout. */
#define FVM_COMPARE_ROOT_COUNT 4U
/* Two operands passed to arithmetic/comparison metadata callbacks. */
#define FVM_COMPARE_META_ARGUMENT_COUNT 2U
/* [GC:] Input string retained through copying and publication. */
#define FVM_GLOBAL_REGISTER_INPUT_ROOT 0U
/* [GC:] Number of retained values in the global_register service layout. */
#define FVM_GLOBAL_REGISTER_ROOT_COUNT 1U
/* [GC:] Explicit native timeline during frame changes. */
#define FVM_FRAME_CONTROL_CLIP_ROOT 0U
/* [GC:] Optional frame or label request. */
#define FVM_FRAME_CONTROL_FRAME_ROOT 1U
/* [GC:] Number of retained values in the frame_control service layout. */
#define FVM_FRAME_CONTROL_ROOT_COUNT 2U
/* [GC:] Original trace operand during toString execution. */
#define FVM_TRACE_INPUT_ROOT 0U
/* [GC:] Number of retained values in the trace service layout. */
#define FVM_TRACE_ROOT_COUNT 1U
/* Positional arguments delivered to the trace host sink. */
#define FVM_TRACE_SINK_ARGUMENT_COUNT 1U
/* [GC:] Original or converted frame request. */
#define FVM_CALL_FRAME_ARGUMENT_ROOT 0U
/* [GC:] Explicit scope receiver during request coercion. */
#define FVM_CALL_FRAME_RECEIVER_ROOT 1U
/* [GC:] Number of retained values in the call_frame service layout. */
#define FVM_CALL_FRAME_ROOT_COUNT 2U
/* [GC:] Converted URL; first host callback argument. */
#define FVM_URL_URL_TEXT_ROOT 0U
/* [GC:] Converted target; second host callback argument. */
#define FVM_URL_TARGET_TEXT_ROOT 1U
/* [GC:] Request flags; third host callback argument. */
#define FVM_URL_FLAGS_ROOT 2U
/* [GC:] Original URL retained through target conversion. */
#define FVM_URL_URL_INPUT_ROOT 3U
/* [GC:] Original target retained through its conversion. */
#define FVM_URL_TARGET_INPUT_ROOT 4U
/* [GC:] Number of retained values in the url service layout. */
#define FVM_URL_ROOT_COUNT 5U
/* Positional arguments delivered to the url host sink. */
#define FVM_URL_SINK_ARGUMENT_COUNT 3U
/* [GC:] Left operand during numeric/metamethod callbacks. */
#define FVM_NUMERIC_LEFT_ROOT 0U
/* [GC:] Right operand during numeric/metamethod callbacks. */
#define FVM_NUMERIC_RIGHT_ROOT 1U
/* [GC:] Number of retained values in the numeric service layout. */
#define FVM_NUMERIC_ROOT_COUNT 2U
/* Two operands passed to arithmetic/comparison metadata callbacks. */
#define FVM_NUMERIC_META_ARGUMENT_COUNT 2U
/* [GC:] Snapshotted capture during generated execution. */
#define FVM_NATIVE_BODY_CAPTURE_ROOT 0U
/* [GC:] Prepared implicit this during generated execution. */
#define FVM_NATIVE_BODY_THIS_ROOT 1U
/* [GC:] Suspended caller this until restoration. */
#define FVM_NATIVE_BODY_PREVIOUS_THIS_ROOT 2U
/* [GC:] Number of retained values in the native_body service layout. */
#define FVM_NATIVE_BODY_ROOT_COUNT 3U
/* [GC:] Left operand retained by recursive loose equality. */
#define FVM_EQUALITY_LEFT_ROOT 0U
/* [GC:] Right operand retained by recursive loose equality. */
#define FVM_EQUALITY_RIGHT_ROOT 1U
/* [GC:] Recursive equality owns a separate two-value frame. */
#define FVM_EQUALITY_ROOT_COUNT 2U
/* Defensive recursion bound for repeated object/primitive equality conversion. */
#define FVM_EQUALITY_MAX_COERCION_DEPTH 8U
#endif
