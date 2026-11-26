/* Copyright (c) 2026 bhgv
 * EN: Encode a bounded initializer or one root method and its formal names.
 * RU: Записать ограниченный инициализатор или метод корня с параметрами.
 */
#define VIEW_PARAMETERS 8 /* Maximum number of formal parameters in one root callback. */

static char method[256];                          /* Root callback name for this unit. */
static char parameters[VIEW_PARAMETERS][256];     /* Bounded formal names in call order. */
static unsigned parameter_count;                  /* Initialized prefix of parameters. */


/* EN: Write source provenance and one unique module name.
 * RU: Записать происхождение исходника и уникальное имя модуля. */
static void header(const char *suffix)
{
    append("FIR 1.0\nmodule @ide_view%s\nsource_language as2\nsource_semantics avm1_numeric\n", suffix);
}


/* EN: Begin the FIR entry block for an initializer.
 * RU: Открыть входной блок FIR для инициализатора. */
void emit_initializer(void)
{
    header("");
    append("func @main() -> void {\n^entry:\n");
}


/* EN: Encode the root method's exact name separately from its unique module spelling.
 * RU: Записать точное имя метода корня отдельно от уникального имени модуля. */
void emit_frame_handler(void)
{
    parameter_count = 0;
    if (!name(method) || strlen(method) > 244)
    {
        failed = 1;
    }
}


/* EN: Copy one distinct formal name from the consumed token.
 * RU: Скопировать одно уникальное имя формального параметра из токена. */
void emit_parameter(void)
{
    char key[256];
    if (parameter_count == VIEW_PARAMETERS || !name(key))
    {
        failed = 1;
        return;
    }
    for (unsigned i = 0; i < parameter_count; ++i)
    {
        if (!strcmp(key, parameters[i]))
        {
            failed = 1;
            return;
        }
    }
    strcpy(parameters[parameter_count++], key);
}


/* EN: Describe the root installation and its ordered formal names to the AVM1 backend.
 * RU: Передать бэкенду AVM1 имя метода корня и порядок формальных параметров. */
void emit_frame_body(void)
{
    char suffix[256] = {0};
    if (strcmp(method, "onEnterFrame"))
    {
        snprintf(suffix, sizeof(suffix), "_%s", method);
    }
    header(suffix);
    if (!*suffix && !parameter_count)
    {
        append("source_entry avm1_frame\n");
    }
    else
    {
        append("source_entry avm1_method @%s", method);
        for (unsigned i = 0; i < parameter_count; ++i)
        {
            append(" @%s", parameters[i]);
        }
        append("\n");
    }
    append("func @main() -> void {\n^entry:\n");
}


/* EN: Return the evaluated expression from the method to its AVM1 caller.
 * RU: Вернуть вычисленное выражение вызывающему методу AVM1. */
void emit_value_return(void)
{
    append("    return %%v%d\n", pop());
}


/* EN: End the body without an explicit result.
 * RU: Завершить тело без явного результата. */
void emit_void_return(void)
{
    append("    return\n");
}
