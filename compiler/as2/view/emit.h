#ifndef AS2_VIEW_EMIT_H
#define AS2_VIEW_EMIT_H
/* Capture current ASCII target token; no runtime global is read for a simple assignment. */
void emit_target(void);
/* Capture member name and emit a FIR read of the LHS owner before its RHS. */
void emit_property(void);
/* Emit a named global read and push its SSA identity on the bounded expression stack. */
void emit_global(void);
/* Emit a checked int32 constant from the current token, then push its SSA identity. */
void emit_number(void);
/* Emit the current bounded string token as a FIR literal. */
void emit_string(void);
/* Capture the current method name in the parser's caller-owned buffer. */
void emit_method_name(char key[256]);
/* Replace the receiver and its optional argument with the observable call result. */
void emit_method_call(const char *key, int argc);
/* Emit a literal boolean constant, then push its SSA identity. */
void emit_boolean(int value);
/* Combine the two expression operands: 0 AS addition; 1/2/3 numeric sub/mul/div. */
void emit_binary(int operation);
/* Finish the current assignment; member selects object property versus global destination. */
void emit_assign(int member);
#endif
