/* Copyright (c) 2026 bhgv
 * EN: Parser actions and assignment target kinds for AS2 FIR lowering.
 * RU: Действия парсера и виды целей присваивания при понижении AS2 в FIR.
 */
#ifndef AS2_VIEW_EMIT_H
#define AS2_VIEW_EMIT_H

#define AS2_TARGET_GLOBAL 0  /* No member suffix on the assignment target. */
#define AS2_TARGET_NAMED 1   /* A literal name following a dot. */
#define AS2_TARGET_KEY 2     /* A captured key from the expression stack. */

/* EN/RU: Mark a regular initializer / Начать обычный инициализатор. */
void emit_initializer(void);
/* EN/RU: Validate the callback name and mark a frame handler / Начать обработчик кадра. */
void emit_frame_handler(void);
/* EN/RU: Emit a branch and return its label index / Выдать ветвление и вернуть номер метки. */
int emit_if(void);
/* EN/RU: Close the true arm and enter the false arm / Перейти с истинной ветви на ложную. */
void emit_else(int label);
/* EN/RU: Close the false arm and enter the merge / Завершить ложную ветвь и выдать слияние. */
void emit_merge(int label);
/* EN/RU: Open a condition block and return its label / Открыть блок условия и вернуть метку. */
int emit_while(void);
/* EN/RU: Enter the loop body on a truthy condition / Войти в тело при истинном условии. */
void emit_while_test(int label);
/* EN/RU: Return to the condition and open the exit / Вернуться к условию и открыть выход. */
void emit_while_end(int label);
/* Capture current ASCII target token; no runtime global is read for a simple assignment. */
void emit_target(void);
/* Capture member name and emit a FIR read of the LHS owner before its RHS. */
void emit_property(void);
/* EN: Capture the LHS owner before parsing a computed key.
 * RU: Захватить владельца левой части до разбора вычисляемого ключа. */
void emit_owner(void);
/* Emit a named global read and push its SSA identity on the bounded expression stack. */
void emit_global(void);

/* EN: Push the namespace and the named owner for a reference capture.
 * RU: Поместить пространство каналов и именованного владельца для захвата ссылки. */
void emit_ref_owner(const char *binding);

/* EN: Push namespace, owner and literal key. member selects dot or root syntax.
 * RU: Поместить пространство, владельца и ключ-литерал. member выбирает точку или корень. */
void emit_ref_named(const char *binding, int member);

/* EN: Test whether the next token begins contextual ref syntax without consuming it.
 * RU: Проверить начало контекстного ref без чтения токена. */
int emit_ref_ahead(void);

/* EN: Consume the owner and evaluated key, then push the result of reading the member.
 * RU: Снять владельца и вычисленный ключ, затем поместить результат чтения поля. */
void emit_member_read(void);

/* EN: Read a literal key on the owner already on the expression stack.
 * RU: Прочитать ключ-литерал у владельца на стеке выражений. */
void emit_named_read(const char *key);
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
/* EN/RU: Negate AS truthiness / Инвертировать логическое значение AS. */
void emit_not(void);
/* Combine the two expression operands: 0 AS addition; 1/2/3 numeric sub/mul/div. */
void emit_binary(int operation);
/* EN/RU: Commit the assignment using kind. / Выпустить присваивание с видом цели kind. */
void emit_assign(int kind);
#endif
