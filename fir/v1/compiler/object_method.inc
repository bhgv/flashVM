/* Copyright (c) 2026 bhgv
 * EN: Encode the zero-argument method installed by a SWO initializer.
 * RU: Записать метод без параметров, устанавливаемый инициализатором SWO.
 * EN/RU: Included by object.c after the AVM1 byte helpers. / Включается из object.c после функций записи.
 */

/* EN: Stage an anonymous zero-argument function after its root receiver and key.
 * RU: Записать заголовок анонимной функции после корневого объекта и имени. */
static size_t frame_begin(Bytes *b, const char *key)
{
    size_t offset;
    text(b, "_root");
    byte(b, 0x1c); /* GetVariable */
    text(b, key);
    offset = b->n;
    byte(b, 0x9b); /* DefineFunction */
    word(b, 0); /* Patched payload size. */
    byte(b, 0); /* Empty function name. */
    word(b, 0); /* Zero parameters. */
    word(b, 0); /* Patched body size. */
    return offset;
}


/* EN: Complete the nested action length and install the handler on the root.
 * RU: Записать длину вложенного блока и установить обработчик на корневом клипе. */
static int frame_end(Bytes *b, size_t offset)
{
    size_t length = b->n - offset - 3;
    size_t body = length - 5;
    if (length > 65535u)
    {
        return 0;
    }
    b->data[offset + 1] = (unsigned char)length;
    b->data[offset + 2] = (unsigned char)(length >> 8);
    b->data[offset + 6] = (unsigned char)body;
    b->data[offset + 7] = (unsigned char)(body >> 8);
    byte(b, 0x4f); /* SetMember */
    byte(b, 0); /* End initializer, after the frame body already ended. */
    return !b->failed;
}
