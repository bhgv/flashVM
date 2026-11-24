/* Copyright (c) 2026 bhgv
 * EN: Create a TextField and publish it through the shared MovieClip depth table.
 * RU: Создание TextField и публикация через общую таблицу глубин MovieClip.
 */

/* EN: Coerce a counted name for the MovieClip binding. / RU: Привести имя привязки MovieClip. */
FVM_Result clip_name(FVM_Context *, const FVM_Value *, FVM_Value &);


/* [GC:] EN: Admit six AVM1 operands, prepare the field and publish one complete depth entry.
 * RU: Принять шесть аргументов AVM1, подготовить TextField и опубликовать готовую глубину. */
FVM_Result create_text_field(FVM_Context *c, FVM_Object *clip, unsigned count, const FVM_Value *values,
                            FVM_Value *out, void *)
{
    if (count < 6)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *clip_data = 0;
    FVM_Result result = state(clip, clip_data);
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_NativeCallGuard guard(c);
    FVM_Value name;
    result = clip_name(c, values, name);
    double position[5];
    for (unsigned int index = 0; index < 5 && result == FVM_OK; ++index)
    {
        result = args(c, count, values, index + 1, 0, position[index]);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    const double depth = position[0];
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH || depth != std::floor(depth))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *text = 0;
    result = fvm_textfield_create(c, 0, position[3], position[4], &text);
    if (result == FVM_OK)
    {
        FVM_Value x = number(position[1]), y = number(position[2]);
        result = fvm_object_set(c, text, "_x", &x);
        if (result == FVM_OK)
        {
            result = fvm_object_set(c, text, "_y", &y);
        }
    }
    FVM_Object *node = 0;
    if (result == FVM_OK)
    {
        result = make(c, name.string, 1, clip->prototype, &node);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    /* [GC:] The private depth node retains the TextField through an ordinary managed edge.
     * Внутренний клип удерживает TextField обычной управляемой ссылкой. */
    FVM_Object *data = node->resource_result.object;
    data->properties["static_art"] = number(1);
    data->properties["text_field"] = object(text);
    result = publish_child(c, clip, node, name.string, (long)depth);
    if (result == FVM_OK)
    {
        *out = object(text);
    }
    return result;
}
