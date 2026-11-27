/* Copyright (c) 2026 bhgv
 * EN: Encode the root method installed by a SWO initializer.
 * RU: Записать метод корня, устанавливаемый инициализатором SWO.
 * EN/RU: Included by object.c after the AVM1 byte helpers. / Включается из object.c после функций записи.
 */

/* EN: Stage an anonymous method with ordered formal names and record its body start.
 * RU: Записать анонимный метод с именами параметров и сохранить начало тела. */
static size_t frame_begin(Bytes *b, const FirModule *module, size_t *body)
{
    size_t offset;
    text(b, "_root");
    byte(b, 0x1c); /* GetVariable */
    text(b, module->avm1_method[0] ? module->avm1_method + 1 : "onEnterFrame");
    offset = b->n;
    byte(b, 0x9b); /* DefineFunction */
    word(b, 0); /* Patched payload size. */
    byte(b, 0); /* Empty function name. */
    word(b, module->avm1_parameter_count);
    for (unsigned p = 0; p < module->avm1_parameter_count; ++p)
    {
        const char *name = module->avm1_parameters[p] + 1;
        do
        {
            byte(b, (unsigned char)*name);
        } while (*name++);
    }
    word(b, 0); /* Patched body size. */
    *body = b->n;
    return offset;
}


/* EN: Complete the nested action length and install the handler on the root.
 * RU: Записать длину вложенного блока и установить обработчик на корневом клипе. */
static int frame_end(Bytes *b, const FirModule *module, size_t offset, size_t body_begin)
{
    size_t length = b->n - offset - 3;
    size_t body = b->n - body_begin;
    if (length > 65535u)
    {
        return 0;
    }
    b->data[offset + 1] = (unsigned char)length;
    b->data[offset + 2] = (unsigned char)(length >> 8);
    b->data[body_begin - 2] = (unsigned char)body;
    b->data[body_begin - 1] = (unsigned char)(body >> 8);
    if (module->avm1_asinc)
    {
        integer(b, 1); /* Async.wrap takes the anonymous function from the stack. */
        text(b, "Async");
        byte(b, 0x1c); /* GetVariable */
        text(b, "wrap");
        byte(b, 0x52); /* CallMethod */
    }
    byte(b, 0x4f); /* SetMember */
    byte(b, 0); /* End initializer, after the frame body already ended. */
    return !b->failed;
}
