/* Copyright (c) 2026 bhgv
 * EN: Opt-in presentation revisions for remotely committed scenes.
 * RU: Ревизии отрисовки для сцен, подтверждённых удалённым владельцем.
 */
#define PAIRED_FRAME_REVISION_MAX 4294967295.0 /* Portable uint32 limit in exact AVM1 numbers. */


/* EN: Decode a whole revision; zero selects continuous rendering.
 * RU: Декодировать целую ревизию. Ноль выбирает непрерывную отрисовку. */
static int valid_frame_revision(const FVM_Value *value)
{
    return value->type == FVM_VALUE_NUMBER && value->number >= 0 &&
        value->number <= PAIRED_FRAME_REVISION_MAX && value->number == (unsigned long)value->number;
}


/* EN: Publish the complete scene's revision without entering SDL or rendering inside the script.
 * RU: Опубликовать ревизию полного снимка без входа в SDL и отрисовки из скрипта. */
static FVM_Result publish_frame(FVM_Context *c, FVM_Object *object, unsigned count,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)state;
    if (count != 1 || !valid_frame_revision(args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, 1);
    return input_number(c, object, "frameRevision", args[0].number);
}


/* EN: Read the current movie's optional revision between owner dispatches.
 * RU: Прочитать необязательную ревизию текущего фильма между обработками владельца. */
static unsigned long frame_revision(FVM_Context *c)
{
    FVM_Value object, revision;
    if (fvm_get_global(c, "hostInput", &object) == FVM_OK && object.type == FVM_VALUE_OBJECT &&
        fvm_object_get(c, object.object, "frameRevision", &revision) == FVM_OK && valid_frame_revision(&revision))
    {
        return (unsigned long)revision.number;
    }
    return 0;
}
