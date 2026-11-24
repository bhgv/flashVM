/* Copyright (c) 2026 bhgv
 * EN: Read scalar length and set a display-only selection on a retained native TextField.
 * RU: Чтение длины в символах Unicode и установка видимого выделения TextField.
 */

/* EN: Count Unicode scalars in the built-in field. Never return UTF-16 positions here.
 * RU: Посчитать символы Unicode в нативном поле. Результат отличается от индекса UTF-16. */
static FVM_Result field_scalar_count(FVM_Context *, FVM_Object *self, unsigned count,
    const FVM_Value *, FVM_Value *out, void *)
{
    const Field *field = (const Field *)self->resource;
    size_t scalars;
    if (count || self->resource_kind != TEXT_FIELD || !field ||
        fvm_utf8_validate(field->text, field->text_size, &scalars) != FVM_UTF8_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(out, (double)scalars);
    return FVM_OK;
}


/* EN: Copy anchor, caret and colors; a negative endpoint hides the preview. No text is edited.
 * RU: Скопировать границы и цвета. Отрицательная граница скрывает выделение, текст не меняется. */
static FVM_Result field_selection_preview(FVM_Context *c, FVM_Object *self, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *)
{
    Field *field = (Field *)self->resource;
    if (count != 4 || self->resource_kind != TEXT_FIELD || !field)
    {
        return FVM_INVALID_ARGUMENT;
    }
    double values[4];
    for (unsigned int i = 0; i < 4; ++i)
    {
        FVM_Result result = number(c, args[i], values[i]);
        if (result != FVM_OK)
        {
            return result;
        }
        const double low = i < 2 ? -1 : 0;
        const double high = i < 2 ? FVM_TEXT_FIELD_MAX_UNITS : 0xffffff;
        if (!finite_range(values[i], low, high) || std::floor(values[i]) != values[i])
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    field->preview = values[0] >= 0 && values[1] >= 0;
    if (field->preview)
    {
        field->preview_anchor = (size_t)values[0];
        field->preview_caret = (size_t)values[1];
        field->preview_fill = values[2];
        field->preview_ink = values[3];
    }
    fvm_value_undefined(out);
    return FVM_OK;
}
