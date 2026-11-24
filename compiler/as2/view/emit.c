/* AS2 display lowering to textual FIR. Target instruction encoding belongs to FIR backends. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "viewp.h"
#include "views.h"
#include "emit.h"
#include "view.h"
#include "frontend.h"

static unsigned char fir_text[65536];             /* Private FIR staging buffer; published only on complete success. */
static size_t used;                               /* Number of staged FIR bytes. */
static int failed;                                /* Sticky host, parser or emission failure. */
static int values;                                /* Next unique SSA value identity. */
static int labels;                                /* Next distinct conditional's label identity. */
static int stack[4096];                           /* Expression values, never VM heap pointers. */
static int depth;                                 /* Number of initialized expression-stack entries. */
static char target[256];                          /* Copied current assignment name. */
static char property[256];                        /* Copied member key, when present. */
static int owner;                                 /* Captured LHS object SSA identity, or -1. */
static As2ViewDiagnostic *active;                 /* Borrowed first-error sink for one serialized call. */
extern int view_parser_run(unsigned char *, int); /* Recover generated scanner failures. */
extern void view_parser_reset(void);              /* Reset generated parser diagnostics between calls. */
extern Error_Func Custom_Error;                   /* Executable/library-local generated diagnostic callback. */

/* Append formatted FIR only within the private buffer; exhaustion fails the complete unit. */
static void append(const char *format, ...)
{
    va_list args;
    int count;
    if (failed)
    {
        return;
    }
    va_start(args, format);
    count = vsnprintf((char *)fir_text + used, sizeof(fir_text) - used, format, args);
    va_end(args);
    if (count < 0 || (size_t)count >= sizeof(fir_text) - used)
    {
        failed = 1;
        return;
    }
    used += (size_t)count;
}

/* Copy one already recognized identifier; never truncate a property/global key. */
static int name(char *output)
{
    int index;
    if (S_Len > 253)
    {
        failed = 1;
        return 0;
    }
    for (index = 0; index < S_Len; index++)
    {
        output[index] = (char)S_CurrentCh(S_Pos + index);
    }
    output[index] = 0;
    return 1;
}

/* Push one SSA identity; malformed/error paths never access uninitialized stack entries. */
static void push(int value)
{
    if (depth == 4096)
    {
        failed = 1;
        return;
    }
    stack[depth++] = value;
}

/* Pop one staged SSA identity; generated error recovery is not trusted to preserve arity. */
static int pop(void)
{
    if (!depth)
    {
        failed = 1;
        return 0;
    }
    return stack[--depth];
}


/* EN: Begin the FIR entry block for an initializer.
 * RU: Открыть входной блок FIR для инициализатора. */
void emit_initializer(void)
{
    append("func @main() -> void {\n^entry:\n");
}


/* EN: Require the root timeline callback name before setting target metadata.
 * RU: Проверить имя обработчика корневого клипа перед записью метаданных. */
void emit_frame_handler(void)
{
    char key[256];
    if (!name(key) || strcmp(key, "onEnterFrame"))
    {
        failed = 1;
        return;
    }
    append("source_entry avm1_frame\n");
    emit_initializer();
}


/* EN: Consume an expression and return its boolean SSA identity.
 * RU: Снять значение выражения и вернуть идентификатор логического SSA. */
static int truth(void)
{
    int value = pop();
    int converted = values++;
    append("    %%v%d: bool = as.to_boolean %%v%d\n", converted, value);
    return converted;
}


/* EN: Consume an expression and branch on the converted AS boolean.
 * RU: Снять значение выражения и выполнить ветвление по логическому значению AS. */
int emit_if(void)
{
    int converted = truth();
    int label = labels++;
    append("    if %%v%d, ^then%d, ^else%d\n^then%d:\n", converted, label, label, label);
    return label;
}


/* EN: Push the negated AS truth value without reevaluating the expression.
 * RU: Поместить на стек отрицание логического значения без повторного вычисления. */
void emit_not(void)
{
    int operand = truth();
    int value = values++;
    append("    %%v%d: bool = bool.not %%v%d\n", value, operand);
    push(value);
}


/* EN: Finish the true arm and open the matching false arm.
 * RU: Завершить истинную ветвь и открыть соответствующую ложную. */
void emit_else(int label)
{
    append("    jump ^merge%d\n^else%d:\n", label, label);
}


/* EN: Finish the false arm and enter the shared continuation.
 * RU: Завершить ложную ветвь и перейти к общему продолжению. */
void emit_merge(int label)
{
    append("    jump ^merge%d\n^merge%d:\n", label, label);
}

/* Capture an assignment target; source comments preserve token line/column, not a debug ABI. */
void emit_target(void)
{
    name(target);
    owner = -1;
    append("    // AS2 assignment line %d column %d byte %ld\n", S_Line, S_Col, S_Pos);
}

/* Capture the LHS owner before evaluating RHS, preserving ordinary AS evaluation order. */
void emit_property(void)
{
    name(property);
    owner = values++;
    append("    %%v%d: any = as.global_get \"%s\"\n", owner, target);
}

/* Emit an observable global read; the FIR optimizer must retain accessor effects/order. */
void emit_global(void)
{
    char key[256];
    if (name(key))
    {
        int value = values++;
        append("    %%v%d: any = as.global_get \"%s\"\n", value, key);
        push(value);
    }
}

/* Lower a decimal literal after exact int32 range admission, independent of host long. */
void emit_number(void)
{
    unsigned long number = 0;
    int index;
    int value;
    for (index = 0; index < S_Len; index++)
    {
        unsigned digit = (unsigned)(S_CurrentCh(S_Pos + index) - '0');
        if (number > (2147483647UL - digit) / 10)
        {
            failed = 1;
            return;
        }
        number = number * 10 + digit;
    }
    value = values++;
    append("    %%v%d: i32 = const.i32 %lu\n", value, number);
    push(value);
}

/* EN: Copy validated token quoting into FIR within its literal capacity.
 * RU: Скопировать проверенный строковый токен в пределах размера литерала FIR. */
void emit_string(void)
{
    char literal[258];
    int index;
    int value;
    if (S_Len > 257)
    {
        failed = 1;
        return;
    }
    for (index = 0; index < S_Len; index++)
    {
        literal[index] = (char)S_CurrentCh(S_Pos + index);
    }
    literal[index] = 0;
    value = values++;
    append("    %%v%d: string = const.string %s\n", value, literal);
    push(value);
}

/* EN: Capture the method key in the caller's buffer before parsing its argument.
 * RU: Сохранить имя метода в буфере вызывающего кода до разбора аргумента. */
void emit_method_name(char key[256])
{
    name(key);
}

/* EN: Replace the receiver and argc operands with one observable call result.
 * RU: Заменить получателя и argc аргументов одним результатом вызова. */
void emit_method_call(const char *key, int argc)
{
    int argument = argc ? pop() : -1;
    int receiver = pop();
    int value = values++;
    append("    %%v%d: any = as.member_call %%v%d, \"%s\"", value, receiver, key);
    if (argc)
    {
        append(", %%v%d", argument);
    }
    append("\n");
    push(value);
}

/* Emit a literal bool SSA value; no numeric/target encoding occurs in the frontend. */
void emit_boolean(int truth)
{
    int value = values++;
    append("    %%v%d: bool = const.bool %s\n", value, truth ? "true" : "false");
    push(value);
}

/* Preserve AS coercion: Add2 is dynamic, other arithmetic explicitly converts both operands. */
void emit_binary(int operation)
{
    int right = pop();
    int left = pop();
    int value;
    if (operation != 0)
    {
        int converted_left = values++;
        int converted_right = values++;
        append("    %%v%d: f64 = as.to_number %%v%d\n", converted_left, left);
        append("    %%v%d: f64 = as.to_number %%v%d\n", converted_right, right);
        left = converted_left;
        right = converted_right;
    }
    value = values++;
    append("    %%v%d: %s = %s %%v%d, %%v%d\n",
           value,
           operation ? "f64" : "any",
           operation == 0   ? "as.add"
           : operation == 1 ? "f64.sub"
           : operation == 2 ? "f64.mul"
                            : "f64.div",
           left,
           right);
    push(value);
}

/* Emit an effectful store after RHS evaluation; one statement must leave no expression values. */
void emit_assign(int member)
{
    int value = pop();
    if (member)
    {
        append("    as.member_set %%v%d, \"%s\", %%v%d\n", owner, property, value);
    }
    else
    {
        append("    as.global_set \"%s\", %%v%d\n", target, value);
    }
    if (depth)
    {
        failed = 1;
    }
}

/* Record the first generated syntax/profile failure, copying only scalar source coordinates. */
static void diagnostic_callback(int code, int line, int column, long position)
{
    if (active && !failed)
    {
        active->code = code;
        active->line = line;
        active->column = column;
        active->offset = position;
    }
    failed = 1;
}

/* Compile one bounded display-profile unit after shared AS2 syntax validation.
 * Input/output/diagnostic are caller-owned, no borrowed text is retained. Return 1/0.
 * Calls must be serialized with this library and its shared AS2 syntax frontend. */
int as2_view_compile(const void *bytes,
                     size_t size,
                     unsigned char *output,
                     size_t capacity,
                     size_t *written,
                     As2ViewDiagnostic *diagnostic)
{
    unsigned char source[16385];
    const unsigned char *input = bytes;
    As2Diagnostic syntax;
    size_t index;
    if (written)
    {
        *written = 0;
    }
    if (diagnostic)
    {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
    if (!bytes || !output || !written || size == 0 || size > 16384)
    {
        return 0;
    }
    if (!as2_check(bytes, size, &syntax))
    {
        if (diagnostic)
        {
            diagnostic->code = syntax.code;
            diagnostic->line = syntax.line;
            diagnostic->column = syntax.column;
            diagnostic->offset = syntax.offset;
        }
        return 0;
    }
    /* EN: Use CR/LF boundaries consistently when parsing line comments.
     * RU: Однозначно разделять комментарии и инструкции по границам CR/LF. */
    for (index = 0; index + 2 < size; index++)
    {
        if (input[index] == 0xe2 && input[index + 1] == 0x80 &&
            (input[index + 2] == 0xa8 || input[index + 2] == 0xa9))
        {
            return 0;
        }
    }
    memcpy(source, bytes, size);
    /* A synthetic final newline closes an ordinary EOF line comment. */
    source[size++] = '\n';
    used = 0;
    values = 0;
    labels = 0;
    depth = 0;
    owner = -1;
    failed = 0;
    active = diagnostic;
    append(
        "FIR 1.0\nmodule @ide_view\nsource_language as2\nsource_semantics avm1_numeric\n");
    Custom_Error = diagnostic_callback;
    view_parser_reset();
    if (!view_parser_run(source, (int)size))
    {
        failed = 1;
    }
    Custom_Error = NULL;
    active = NULL;
    append("    return\n}\n");
    if (failed || depth || !Successful() || used > capacity)
    {
        return 0;
    }
    memcpy(output, fir_text, used);
    *written = used;
    return 1;
}
