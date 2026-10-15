/* FlashVM: AVM1 activation, payload and bounded-execution constants. */
#ifndef FVM_AVM1_ENCODING_CONSTANTS_H
#define FVM_AVM1_ENCODING_CONSTANTS_H /* Header guard. */
/* Preload the actual receiver into a private register. */
#define FVM_AVM1_F2_PRELOAD_THIS 1U
/* Suppress the ordinary named this binding. */
#define FVM_AVM1_F2_SUPPRESS_THIS 2U
/* Preload the prepared arguments object. */
#define FVM_AVM1_F2_PRELOAD_ARGUMENTS 4U
/* Suppress the named arguments binding. */
#define FVM_AVM1_F2_SUPPRESS_ARGUMENTS 8U
/* Preload the prepared super facade. */
#define FVM_AVM1_F2_PRELOAD_SUPER 16U
/* Suppress the named super binding. */
#define FVM_AVM1_F2_SUPPRESS_SUPER 32U
/* Preload the display root. */
#define FVM_AVM1_F2_PRELOAD_ROOT 64U
/* Preload the parent clip when available. */
#define FVM_AVM1_F2_PRELOAD_PARENT 128U
/* Preload the global object. */
#define FVM_AVM1_F2_PRELOAD_GLOBAL 256U
/* Either special this preload/suppression flag. */
#define FVM_AVM1_F2_THIS_FLAGS (FVM_AVM1_F2_PRELOAD_THIS | FVM_AVM1_F2_SUPPRESS_THIS)
/* Either special arguments preload/suppression flag. */
#define FVM_AVM1_F2_ARGUMENTS_FLAGS (FVM_AVM1_F2_PRELOAD_ARGUMENTS | FVM_AVM1_F2_SUPPRESS_ARGUMENTS)
/* Either special super preload/suppression flag. */
#define FVM_AVM1_F2_SUPER_FLAGS (FVM_AVM1_F2_PRELOAD_SUPER | FVM_AVM1_F2_SUPPRESS_SUPER)
/* Either display-root or parent preload request. */
#define FVM_AVM1_F2_DISPLAY_FLAGS (FVM_AVM1_F2_PRELOAD_ROOT | FVM_AVM1_F2_PRELOAD_PARENT)
/* All nine Function2 preload/suppression bits; other bits are reserved. */
#define FVM_AVM1_F2_VALID_FLAGS 0x1ffU
/* Try record contains a catch block. */
#define FVM_AVM1_TRY_CATCH 1U
/* Try record contains a finally block. */
#define FVM_AVM1_TRY_FINALLY 2U
/* Catch target is a register byte instead of a terminated name. */
#define FVM_AVM1_TRY_CATCH_REGISTER 4U
/* All valid Try flags; higher bits are reserved. */
#define FVM_AVM1_TRY_VALID_FLAGS 7U
/* Three consecutive blocks: try, catch and finally. */
#define FVM_AVM1_TRY_BLOCK_COUNT 3U
/* Index of the protected try block. */
#define FVM_AVM1_TRY_BODY_INDEX 0U
/* Index of the exception handler block. */
#define FVM_AVM1_TRY_CATCH_INDEX 1U
/* Index of the cleanup block. */
#define FVM_AVM1_TRY_FINALLY_INDEX 2U
/* Maximum nested script/coercion depth before bounded execution fails. */
#define FVM_AVM1_MAX_SCRIPT_DEPTH 64U
/* Maximum nested encoded function/try-block inspection depth. */
#define FVM_AVM1_MAX_BLOCK_DEPTH 64U
/* Maximum accepted encoded call argument count. */
#define FVM_AVM1_MAX_CALL_ARGUMENTS 65535U
/* Bound on materialized Array/Object literal entries. */
#define FVM_AVM1_MAX_LITERAL_ENTRIES 65536UL
/* Register slots prepared for a legacy DefineFunction body. */
#define FVM_AVM1_LEGACY_REGISTER_COUNT 4U
/* Largest Function2 register/count byte value. */
#define FVM_AVM1_MAX_REGISTER_INDEX 255U
/* Low-bit mask implementing polling every 64 semantic steps. */
#define FVM_AVM1_POLL_INTERVAL_MASK 63UL
/* Low two request bits select none, GET or POST. */
#define FVM_AVM1_URL_METHOD_MASK 3U
/* Reserved HTTP method bit combination. */
#define FVM_AVM1_URL_INVALID_METHOD 3U
/* GetURL2 carries one encoded request-policy byte after its Action header. */
#define FVM_AVM1_URL_FLAGS_BYTES 1U
/* Legacy URL payloads currently support only ASCII before SWF6. */
#define FVM_AVM1_URL_LEGACY_MAX_BYTE 0x7fU
/* Reserved GetURL2 payload bits rejected during validation. */
#define FVM_AVM1_URL_RESERVED_MASK 0x3cU
/* HTTP method bits plus load-target and load-variables request bits. */
#define FVM_AVM1_URL_VALID_FLAGS 195U
/* GotoFrame2 enables playback after seeking. */
#define FVM_AVM1_GOTO_PLAY 1U
/* GotoFrame2 carries a two-byte scene frame bias. */
#define FVM_AVM1_GOTO_SCENE_BIAS 2U
/* Valid GotoFrame2 playback and scene-bias bits. */
#define FVM_AVM1_GOTO_VALID_FLAGS 3U
/* ActionPush operand discriminator for string. */
#define FVM_AVM1_PUSH_STRING 0U
/* ActionPush operand discriminator for float. */
#define FVM_AVM1_PUSH_FLOAT 1U
/* ActionPush operand discriminator for null. */
#define FVM_AVM1_PUSH_NULL 2U
/* ActionPush operand discriminator for undefined. */
#define FVM_AVM1_PUSH_UNDEFINED 3U
/* ActionPush operand discriminator for register. */
#define FVM_AVM1_PUSH_REGISTER 4U
/* ActionPush operand discriminator for boolean. */
#define FVM_AVM1_PUSH_BOOLEAN 5U
/* ActionPush operand discriminator for double. */
#define FVM_AVM1_PUSH_DOUBLE 6U
/* ActionPush operand discriminator for integer. */
#define FVM_AVM1_PUSH_INTEGER 7U
/* ActionPush operand discriminator for constant8. */
#define FVM_AVM1_PUSH_CONSTANT8 8U
/* ActionPush operand discriminator for constant16. */
#define FVM_AVM1_PUSH_CONSTANT16 9U
/* Maximum retained body size accepted by the standalone script-function constructor. */
#define FVM_AVM1_MAX_BODY_BYTES 65536UL
/* ActionWith carries a UI16 body size before its nested bytecode. */
#define FVM_AVM1_WITH_HEADER_BYTES 2U
/* Number of implicit Function2 preload values, in their fixed assignment order. */
#define FVM_AVM1_PRELOAD_COUNT 6U
/* Index of the receiver among implicit preload values. */
#define FVM_AVM1_PRELOAD_THIS_INDEX 0U
/* Index of the arguments object among implicit preload values. */
#define FVM_AVM1_PRELOAD_ARGUMENTS_INDEX 1U
/* Index of the superclass facade among implicit preload values. */
#define FVM_AVM1_PRELOAD_SUPER_INDEX 2U
/* Index of the display root among implicit preload values. */
#define FVM_AVM1_PRELOAD_ROOT_INDEX 3U
/* Index of the display parent among implicit preload values. */
#define FVM_AVM1_PRELOAD_PARENT_INDEX 4U
/* Index of the global object among implicit preload values. */
#define FVM_AVM1_PRELOAD_GLOBAL_INDEX 5U
/* Private function graphs encode each byte as two ASCII hex digits. */
#define FVM_AVM1_HEX_DIGITS_PER_BYTE 2UL
/* Maximum byte length of that ASCII body representation. */
#define FVM_AVM1_MAX_HEX_BODY_BYTES (FVM_AVM1_MAX_BODY_BYTES * FVM_AVM1_HEX_DIGITS_PER_BYTE)
/* Unloaded compatibility frames include slot zero in addition to the highest declared index. */
#define FVM_AVM1_HOST_REGISTER_EXTRA_SLOT 1U
/* Direct frame check stores a UI16 frame followed by a UI8 record-skip count. */
#define FVM_AVM1_WAIT_DIRECT_BYTES 3U
/* Value frame check stores only a UI8 record-skip count; frame comes from a value. */
#define FVM_AVM1_WAIT_VALUE_BYTES 1U
/* Skip byte follows the direct frame's UI16 bytes. */
#define FVM_AVM1_WAIT_DIRECT_SKIP_OFFSET 2U
#endif
