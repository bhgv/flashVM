/* FlashVM: AVM1 wire Action identifiers; values remain distinct from public execution commands. */
#ifndef FVM_AVM1_ACTION_CODES_H
#define FVM_AVM1_ACTION_CODES_H /* Header guard. */
/* Terminate the current action block. */
#define FVM_AVM1_ACTION_END 0x00U
/* Advance the selected timeline by one frame. */
#define FVM_AVM1_ACTION_NEXT_FRAME 0x04U
/* Rewind the selected timeline by one frame. */
#define FVM_AVM1_ACTION_PREVIOUS_FRAME 0x05U
/* Start playback of the selected timeline. */
#define FVM_AVM1_ACTION_PLAY 0x06U
/* Stop playback of the selected timeline. */
#define FVM_AVM1_ACTION_STOP 0x07U
/* Legacy numeric addition. */
#define FVM_AVM1_ACTION_ADD 0x0aU
/* Numeric subtraction. */
#define FVM_AVM1_ACTION_SUBTRACT 0x0bU
/* Numeric multiplication. */
#define FVM_AVM1_ACTION_MULTIPLY 0x0cU
/* Numeric division, including the legacy error string. */
#define FVM_AVM1_ACTION_DIVIDE 0x0dU
/* Legacy numeric equality. */
#define FVM_AVM1_ACTION_EQUALS 0x0eU
/* Legacy numeric less-than comparison. */
#define FVM_AVM1_ACTION_LESS 0x0fU
/* Convert both operands to truth and combine with AND. */
#define FVM_AVM1_ACTION_AND 0x10U
/* Convert both operands to truth and combine with OR. */
#define FVM_AVM1_ACTION_OR 0x11U
/* Invert the converted truth value. */
#define FVM_AVM1_ACTION_NOT 0x12U
/* Compare converted strings for equality. */
#define FVM_AVM1_ACTION_STRING_EQUALS 0x13U
/* Count the supported string representation units. */
#define FVM_AVM1_ACTION_STRING_LENGTH 0x14U
/* Extract a one-based string range. */
#define FVM_AVM1_ACTION_STRING_EXTRACT 0x15U
/* Discard the top operand. */
#define FVM_AVM1_ACTION_POP 0x17U
/* Convert an operand through signed Int32 wrapping. */
#define FVM_AVM1_ACTION_TO_INTEGER 0x18U
/* Resolve a named variable in the active scope. */
#define FVM_AVM1_ACTION_GET_VARIABLE 0x1cU
/* Assign a named variable in the active scope. */
#define FVM_AVM1_ACTION_SET_VARIABLE 0x1dU
/* Select the display target from a stack value. */
#define FVM_AVM1_ACTION_SET_TARGET2 0x20U
/* Concatenate converted strings. */
#define FVM_AVM1_ACTION_STRING_ADD 0x21U
/* Read an indexed intrinsic display property. */
#define FVM_AVM1_ACTION_GET_PROPERTY 0x22U
/* Write an indexed intrinsic display property. */
#define FVM_AVM1_ACTION_SET_PROPERTY 0x23U
/* Duplicate a resolved display source at an absolute display depth. */
#define FVM_AVM1_ACTION_CLONE_SPRITE 0x24U
/* Resolve and remove a dynamically eligible display clip. */
#define FVM_AVM1_ACTION_REMOVE_SPRITE 0x25U
/* Convert and deliver a trace value. */
#define FVM_AVM1_ACTION_TRACE 0x26U
/* Compare converted strings for ordering. */
#define FVM_AVM1_ACTION_STRING_LESS 0x29U
/* Raise the supplied ActionScript value. */
#define FVM_AVM1_ACTION_THROW 0x2aU
/* Apply an interface/class cast. */
#define FVM_AVM1_ACTION_CAST 0x2bU
/* Declare implemented interfaces. */
#define FVM_AVM1_ACTION_IMPLEMENTS 0x2cU
/* Draw an integer below a numerically coerced, nonnegative Int32 bound. */
#define FVM_AVM1_ACTION_RANDOM_NUMBER 0x30U
/* Multibyte length variant under the supported version policy. */
#define FVM_AVM1_ACTION_MB_STRING_LENGTH 0x31U
/* Convert a string unit to its numeric character code. */
#define FVM_AVM1_ACTION_CHAR_TO_ASCII 0x32U
/* Convert a numeric character code to text. */
#define FVM_AVM1_ACTION_ASCII_TO_CHAR 0x33U
/* Push elapsed integer milliseconds modulo 2^32. */
#define FVM_AVM1_ACTION_GET_TIME 0x34U
/* Multibyte extraction variant under the supported version policy. */
#define FVM_AVM1_ACTION_MB_STRING_EXTRACT 0x35U
/* Multibyte character-code extraction variant. */
#define FVM_AVM1_ACTION_MB_CHAR_TO_ASCII 0x36U
/* Multibyte character construction variant. */
#define FVM_AVM1_ACTION_MB_ASCII_TO_CHAR 0x37U
/* Delete an own member from an explicit object. */
#define FVM_AVM1_ACTION_DELETE 0x3aU
/* Delete a scoped binding by a coercible name and return whether it was removed. */
#define FVM_AVM1_ACTION_DELETE2 0x3bU
/* Create a named local binding with a value. */
#define FVM_AVM1_ACTION_DEFINE_LOCAL 0x3cU
/* Invoke a named function with supplied operands. */
#define FVM_AVM1_ACTION_CALL_FUNCTION 0x3dU
/* Return the top operand from the current function. */
#define FVM_AVM1_ACTION_RETURN 0x3eU
/* Calculate floating-point remainder. */
#define FVM_AVM1_ACTION_MODULO 0x3fU
/* Construct a named object. */
#define FVM_AVM1_ACTION_NEW_OBJECT 0x40U
/* Create an undefined local binding. */
#define FVM_AVM1_ACTION_DEFINE_LOCAL2 0x41U
/* Construct an Array literal from stack values. */
#define FVM_AVM1_ACTION_INIT_ARRAY 0x42U
/* Construct an Object literal from name/value pairs. */
#define FVM_AVM1_ACTION_INIT_OBJECT 0x43U
/* Report the AVM1 type label. */
#define FVM_AVM1_ACTION_TYPE_OF 0x44U
/* Return the dot path of a boxed display object, or undefined. */
#define FVM_AVM1_ACTION_TARGET_PATH 0x45U
/* Enumerate a named variable object. */
#define FVM_AVM1_ACTION_ENUMERATE 0x46U
/* Dynamic primitive/string-aware addition. */
#define FVM_AVM1_ACTION_ADD2 0x47U
/* Version-aware relational comparison. */
#define FVM_AVM1_ACTION_LESS2 0x48U
/* Version-aware loose equality. */
#define FVM_AVM1_ACTION_EQUALS2 0x49U
/* Convert an operand to Number. */
#define FVM_AVM1_ACTION_TO_NUMBER 0x4aU
/* Convert an operand to String. */
#define FVM_AVM1_ACTION_TO_STRING 0x4bU
/* Duplicate the top operand. */
#define FVM_AVM1_ACTION_PUSH_DUPLICATE 0x4cU
/* Exchange the two top operands. */
#define FVM_AVM1_ACTION_STACK_SWAP 0x4dU
/* Read a value member with counted key handling. */
#define FVM_AVM1_ACTION_GET_MEMBER 0x4eU
/* Write a value member with counted key handling. */
#define FVM_AVM1_ACTION_SET_MEMBER 0x4fU
/* Add one after numeric conversion. */
#define FVM_AVM1_ACTION_INCREMENT 0x50U
/* Subtract one after numeric conversion. */
#define FVM_AVM1_ACTION_DECREMENT 0x51U
/* Invoke an explicit receiver method. */
#define FVM_AVM1_ACTION_CALL_METHOD 0x52U
/* Construct through an explicit receiver member. */
#define FVM_AVM1_ACTION_NEW_METHOD 0x53U
/* Test constructor/prototype membership. */
#define FVM_AVM1_ACTION_INSTANCE_OF 0x54U
/* Enumerate an explicit object value. */
#define FVM_AVM1_ACTION_ENUMERATE2 0x55U
/* 32-bit bitwise AND. */
#define FVM_AVM1_ACTION_BIT_AND 0x60U
/* 32-bit bitwise OR. */
#define FVM_AVM1_ACTION_BIT_OR 0x61U
/* 32-bit bitwise exclusive OR. */
#define FVM_AVM1_ACTION_BIT_XOR 0x62U
/* Masked 32-bit left shift. */
#define FVM_AVM1_ACTION_BIT_LSHIFT 0x63U
/* Signed arithmetic right shift. */
#define FVM_AVM1_ACTION_BIT_RSHIFT 0x64U
/* Logical right shift with version-dependent result interpretation. */
#define FVM_AVM1_ACTION_BIT_URSHIFT 0x65U
/* Same-type equality without coercion. */
#define FVM_AVM1_ACTION_STRICT_EQUALS 0x66U
/* Dynamic greater-than comparison. */
#define FVM_AVM1_ACTION_GREATER 0x67U
/* String greater-than comparison. */
#define FVM_AVM1_ACTION_STRING_GREATER 0x68U
/* Link a subclass constructor/prototype. */
#define FVM_AVM1_ACTION_EXTENDS 0x69U
/* Seek a zero-based encoded frame. */
#define FVM_AVM1_ACTION_GOTO_FRAME 0x81U
/* Deliver a literal URL/target request. */
#define FVM_AVM1_ACTION_GET_URL 0x83U
/* Copy the top operand into a private register. */
#define FVM_AVM1_ACTION_STORE_REGISTER 0x87U
/* Replace the active constant-string pool. */
#define FVM_AVM1_ACTION_CONSTANT_POOL 0x88U
/* Skip a count of Action records if a direct frame is unavailable. */
#define FVM_AVM1_ACTION_WAIT_FOR_FRAME 0x8aU
/* Select a display target from an encoded literal path. */
#define FVM_AVM1_ACTION_SET_TARGET 0x8bU
/* Seek the encoded frame label. */
#define FVM_AVM1_ACTION_GOTO_LABEL 0x8cU
/* Skip a count of Action records if a value-selected frame is unavailable. */
#define FVM_AVM1_ACTION_WAIT_FOR_FRAME2 0x8dU
/* Define a function with registers and preload flags. */
#define FVM_AVM1_ACTION_DEFINE_FUNCTION2 0x8eU
/* Execute length-delimited try/catch/finally blocks. */
#define FVM_AVM1_ACTION_TRY 0x8fU
/* Decode typed literal/register/constant operands. */
#define FVM_AVM1_ACTION_PUSH 0x96U
/* Branch by a signed relative byte offset. */
#define FVM_AVM1_ACTION_JUMP 0x99U
/* Deliver coerced URL/target values and request flags. */
#define FVM_AVM1_ACTION_GET_URL2 0x9aU
/* With executes a bounded body under an additional object scope. */
#define FVM_AVM1_ACTION_WITH 0x94U
/* Define the legacy function record. */
#define FVM_AVM1_ACTION_DEFINE_FUNCTION 0x9bU
/* Branch when the converted operand is true. */
#define FVM_AVM1_ACTION_IF 0x9dU
/* Execute frame actions without seeking. */
#define FVM_AVM1_ACTION_CALL 0x9eU
/* Seek a value-selected frame with playback/scene flags. */
#define FVM_AVM1_ACTION_GOTO_FRAME2 0x9fU
/* Action identifiers at or above this boundary carry a little-endian payload length. */
#define FVM_AVM1_LONG_ACTION_BASE 0x80U
#endif
