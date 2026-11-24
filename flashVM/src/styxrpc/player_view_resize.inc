/* Copyright (c) 2026 bhgv
 * EN: Copy the movie's resize policy and commit viewport dimensions between complete scenes.
 * RU: Копировать правила окна и применять размеры между полными снимками сцены.
 */
#ifdef FVM_STYXRPC_WINDOW
#define PAIRED_EXTENT_BASE (PAIRED_MAX_PIXELS + 1U) /* Exact encoding for a pair of bounded dimensions. */
#define PAIRED_EXTENT_PAIRS (PAIRED_EXTENT_BASE * PAIRED_EXTENT_BASE) /* Number of distinct packed pairs. */


/* EN: Unpack an exact pair of nonzero, bounded dimensions.
 * RU: Распаковать точную пару ненулевых размеров с допустимыми границами. */
static int unpack_extent(unsigned int packed, unsigned int *width, unsigned int *height)
{
    *width = packed / PAIRED_EXTENT_BASE;
    *height = packed % PAIRED_EXTENT_BASE;
    return *width && *width <= PAIRED_MAX_PIXELS && *height;
}


/* EN: Decode a rooted object's option as one exact width/height pair, with no native state changes.
 * RU: Прочитать точную пару ширины и высоты из объекта, не меняя нативное состояние. */
static int input_extent(FVM_Context *c, FVM_Object *object, const char *name,
    unsigned int *width, unsigned int *height)
{
    FVM_Value value;
    unsigned int packed;
    if (fvm_object_get(c, object, name, &value) != FVM_OK || value.type != FVM_VALUE_NUMBER ||
        !(value.number >= 1 && value.number < PAIRED_EXTENT_PAIRS))
    {
        return 0;
    }
    packed = (unsigned int)value.number;
    return (double)packed == value.number && unpack_extent(packed, width, height);
}


/* EN: Read both bounds from one exact numeric property to avoid partial publication.
 * RU: Прочитать обе границы из одного точного значения, исключая частичную публикацию. */
static int resize_bounds(FVM_Context *c, FVM_Object *object, unsigned int bounds[4])
{
    FVM_Value value;
    double low;
    unsigned int high, minimum;
    if (fvm_object_get(c, object, "resizeBounds", &value) != FVM_OK || value.type != FVM_VALUE_NUMBER ||
        !(value.number >= 1 && value.number < (double)PAIRED_EXTENT_PAIRS * PAIRED_EXTENT_PAIRS))
    {
        return 0;
    }
    high = (unsigned int)(value.number / PAIRED_EXTENT_PAIRS);
    low = value.number - (double)high * PAIRED_EXTENT_PAIRS;
    if (!(low >= 1 && low < PAIRED_EXTENT_PAIRS))
    {
        return 0;
    }
    minimum = (unsigned int)low;
    return (double)minimum == low && unpack_extent(minimum, bounds, bounds + 1) &&
        unpack_extent(high, bounds + 2, bounds + 3) && bounds[0] <= bounds[2] && bounds[1] <= bounds[3];
}


/* EN: Check a numeric pixel extent before narrowing to bounded unsigned dimensions.
 * RU: Проверить размер в пикселях до преобразования к целым без знака. */
static int pixel_extent(const FVM_Value *args, unsigned int *width, unsigned int *height)
{
    if (args[0].type != FVM_VALUE_NUMBER || args[1].type != FVM_VALUE_NUMBER ||
        !(args[0].number >= 1 && args[0].number <= PAIRED_MAX_PIXELS) ||
        !(args[1].number >= 1 && args[1].number <= PAIRED_MAX_PIXELS))
    {
        return 0;
    }
    *width = (unsigned int)args[0].number;
    *height = (unsigned int)args[1].number;
    return *width == args[0].number && *height == args[1].number;
}


/* EN: Store a complete policy in the movie. This does not enter SDL during AS execution.
 * RU: Сохранить полные правила окна в фильме. Вызов не входит в SDL при выполнении AS. */
static FVM_Result enable_resize(FVM_Context *c, FVM_Object *object, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    unsigned int minw, minh, maxw, maxh;
    FVM_Result result;
    (void)state;
    if (count != 4 || !pixel_extent(args, &minw, &minh) || !pixel_extent(args + 2, &maxw, &maxh) ||
        minw > maxw || minh > maxh)
    {
        return FVM_INVALID_ARGUMENT;
    }
    result = input_number(c, object, "resizeBounds", (double)(maxw * PAIRED_EXTENT_BASE + maxh) * PAIRED_EXTENT_PAIRS +
        minw * PAIRED_EXTENT_BASE + minh);
    fvm_value_boolean(out, result == FVM_OK);
    return result;
}


/* EN: Publish a complete acknowledged scene extent, consumed at the next render/capture.
 * RU: Опубликовать подтверждённый размер сцены для следующего вывода или снимка. */
static FVM_Result commit_viewport(FVM_Context *c, FVM_Object *object, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    unsigned int width, height, bounds[4];
    (void)state;
    if (count != 2 || !pixel_extent(args, &width, &height) ||
        !resize_bounds(c, object, bounds) ||
        width < bounds[0] || height < bounds[1] || width > bounds[2] || height > bounds[3])
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, 1);
    return input_number(c, object, "canvasExtent", width * PAIRED_EXTENT_BASE + height);
}


/* EN: Borrow the current input object and validate all copied bounds for this render turn.
 * RU: Заимствовать объект ввода и проверить все границы для этого цикла отрисовки. */
static FVM_Object *resize_policy(FVM_Context *c, unsigned int bounds[4])
{
    FVM_Value object, enabled;
    if (fvm_get_global(c, "hostInput", &object) != FVM_OK || object.type != FVM_VALUE_OBJECT ||
        fvm_object_get(c, object.object, "enabled", &enabled) != FVM_OK ||
        enabled.type != FVM_VALUE_BOOLEAN || !enabled.boolean ||
        !resize_bounds(c, object.object, bounds))
    {
        return NULL;
    }
    return object.object;
}


/* EN: Select the last acknowledged canvas when present; otherwise keep the authored extent.
 * RU: Использовать последний подтверждённый размер либо исходный размер фильма. */
static void observe_canvas(FVM_Context *c, unsigned int *width, unsigned int *height)
{
    unsigned int bounds[4], w, h;
    FVM_Object *object = resize_policy(c, bounds);
    if (object && input_extent(c, object, "canvasExtent", &w, &h) &&
        w >= bounds[0] && h >= bounds[1] && w <= bounds[2] && h <= bounds[3])
    {
        *width = w;
        *height = h;
    }
}
#endif
