/* Copyright (c) 2026 bhgv
 * EN: Lower binding reads, assignments and capture of variable references to FIR.
 * RU: Понижать чтение, присваивание и захват ссылок на переменные в FIR.
 */

static char target[256];   /* Current assignment binding copied from the source token. */
static char property[256]; /* Literal member name when the assignment uses dot syntax. */
static int owner;          /* LHS owner SSA identity set before the key or RHS is parsed. */


/* EN: Read one named binding and return its SSA identity.
 * RU: Прочитать именованную переменную и вернуть её идентификатор SSA. */
static int binding_read(const char *key)
{
    int value = values++;
    append("    %%v%d: any = as.global_get \"%s\"\n", value, key);
    return value;
}


/* EN: Emit an already validated identifier as a FIR string and push its identity.
 * RU: Выдать проверенный идентификатор как строку FIR и поместить его на стек. */
static void key_literal(const char *key)
{
    int value = values++;
    append("    %%v%d: string = const.string \"%s\"\n", value, key);
    push(value);
}


/* EN: Capture LHS spelling and source position without evaluating a binding.
 * RU: Сохранить имя и позицию левой части без вычисления переменной. */
void emit_target(void)
{
    name(target);
    append("    // AS2 assignment line %d column %d byte %ld\n", S_Line, S_Col, S_Pos);
}


/* EN: Evaluate the LHS owner before any key expression or RHS.
 * RU: Вычислить владельца левой части до ключа и правой части. */
void emit_owner(void)
{
    owner = binding_read(target);
}


/* EN: Capture the literal LHS member key following a dot and evaluate its owner.
 * RU: Сохранить имя поля левой части после точки и вычислить его владельца. */
void emit_property(void)
{
    name(property);
    emit_owner();
}


/* EN: Read an observable global from the current token onto the expression stack.
 * RU: Прочитать наблюдаемую глобальную переменную из текущего токена на стек. */
void emit_global(void)
{
    char key[256];
    if (name(key))
    {
        push(binding_read(key));
    }
}


/* EN: Recognize contextual ref before a profile variable name.
 * RU: Распознать контекстное ref перед именем переменной профиля. */
int emit_ref_ahead(void)
{
    long cursor = S_NextPos + S_NextLen;
    int ch;
    if (S_NextLen != 3 || S_CurrentCh(S_NextPos) != 'r' ||
        S_CurrentCh(S_NextPos + 1) != 'e' || S_CurrentCh(S_NextPos + 2) != 'f')
    {
        return 0;
    }
    while ((ch = S_CurrentCh(cursor)) == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
        (ch == '/' && S_CurrentCh(cursor + 1) == '/'))
    {
        if (ch == '/')
        {
            do
            {
                ch = S_CurrentCh(++cursor);
            } while (ch > 0 && ch != '\r' && ch != '\n');
            if (ch <= 0)
            {
                return 0;
            }
        }
        ++cursor;
    }
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}


/* EN: Push the evaluated channel namespace and source owner before evaluating a key.
 * RU: Поместить на стек пространство каналов и владельца до вычисления ключа. */
void emit_ref_owner(const char *binding)
{
    push(binding_read("channel"));
    push(binding_read(binding));
}


/* EN: Start a ref call on the root or on binding and push its literal key onto the stack.
 * RU: Начать вызов ref на корне или binding и поместить ключ-литерал на стек. */
void emit_ref_named(const char *binding, int member)
{
    char key[256];
    if (member && !name(key))
    {
        return;
    }
    emit_ref_owner(member ? binding : "_root");
    key_literal(member ? key : binding);
}


/* EN: Consume a captured key and owner, pushing the result of an ordinary member read.
 * RU: Снять захваченный ключ и владельца, поместить результат обычного чтения поля. */
void emit_member_read(void)
{
    int key = pop();
    int receiver = pop();
    int value = values++;
    append("    %%v%d: any = as.member_get %%v%d, %%v%d\n", value, receiver, key);
    push(value);
}


/* EN: Read a literal named member from the owner already on the stack.
 * RU: Прочитать именованное поле владельца на стеке. */
void emit_named_read(const char *key)
{
    key_literal(key);
    emit_member_read();
}


/* EN: Store the RHS once to the captured target selected by kind. Require a balanced stack.
 * RU: Один раз записать правую часть в захваченную цель вида kind. Проверить баланс стека. */
void emit_assign(int kind)
{
    int value = pop();
    switch (kind)
    {
    case AS2_TARGET_KEY:
    {
        int key = emit_arg(-1, pop());
        int operands = emit_arg(key, value);
        append("    as.member_put %%v%d, %%v%d\n", owner, operands);
        break;
    }
    case AS2_TARGET_NAMED:
        append("    as.member_set %%v%d, \"%s\", %%v%d\n", owner, property, value);
        break;
    case AS2_TARGET_GLOBAL:
        append("    as.global_set \"%s\", %%v%d\n", target, value);
        break;
    default:
        failed = 1;
        break;
    }
    if (depth)
    {
        failed = 1;
    }
}
