/* Copyright (c) 2026 bhgv
 * EN: Shared player navigation_bindings implementation; included in owner-thread dependency order.
 * RU: Shared player navigation_bindings; подключается в порядке owner-thread зависимостей.
 */


/* Install loader facades in the optional host only; the standalone VM remains transport-neutral. */
static FVM_Result install_load_functions(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_Value constructor, prototype;
    FVM_Result status;
    unsigned int i;
    for (i = 0; i < FVM_DESKTOP_LOAD_BINDING_COUNT; ++i)
    {
        navigation->builtins[i].navigation = navigation;
        navigation->builtins[i].method = i == FVM_DESKTOP_LOAD_MOVIE_METHOD || i == FVM_DESKTOP_LOAD_VARIABLES_METHOD;
        navigation->builtins[i].flags = i == FVM_DESKTOP_LOAD_VARIABLES_GLOBAL || i == FVM_DESKTOP_LOAD_VARIABLES_METHOD
                                            ? FVM_AVM1_URL_LOAD_VARIABLES
                                            : FVM_AVM1_URL_LOAD_TARGET;
    }
    status = fvm_register_function_rooted_bytes(context,
                                                "loadMovie",
                                                strlen("loadMovie"),
                                                load_builtin,
                                                &navigation->builtins[FVM_DESKTOP_LOAD_MOVIE_GLOBAL],
                                                0);
    if (status == FVM_OK)
    {
        status = fvm_register_function_rooted_bytes(context,
                                                    "loadVariables",
                                                    strlen("loadVariables"),
                                                    load_builtin,
                                                    &navigation->builtins[FVM_DESKTOP_LOAD_VARIABLES_GLOBAL],
                                                    0);
    }
    if (status == FVM_OK)
    {
        status = fvm_get_global(context, "MovieClip", &constructor);
    }
    if (status == FVM_OK && constructor.type == FVM_VALUE_OBJECT)
    {
        status = fvm_object_get(context, constructor.object, "prototype", &prototype);
    }
    else if (status == FVM_OK)
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK && prototype.type == FVM_VALUE_OBJECT)
    {
        status = fvm_object_define_method_rooted(context,
                                                 prototype.object,
                                                 "loadMovie",
                                                 load_builtin,
                                                 &navigation->builtins[FVM_DESKTOP_LOAD_MOVIE_METHOD],
                                                 0);
    }
    else if (status == FVM_OK)
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context,
                                                 prototype.object,
                                                 "loadVariables",
                                                 load_builtin,
                                                 &navigation->builtins[FVM_DESKTOP_LOAD_VARIABLES_METHOD],
                                                 0);
    }
    if (status == FVM_OK)
    {
        status = fvm_register_function_rooted_bytes(
            context, "unloadMovie", strlen("unloadMovie"), unload_builtin, navigation, 0);
    }
    if (status == FVM_OK)
    {
        status =
            fvm_object_define_method_rooted(context, prototype.object, "unloadMovie", unload_builtin, navigation, 0);
    }
    if (status == FVM_OK)
    {
        status = fvm_register_function_rooted_bytes(
            context, "loadMovieNum", strlen("loadMovieNum"), load_numbered_builtin, navigation, 0);
    }
    if (status == FVM_OK)
    {
        status = fvm_register_function_rooted_bytes(
            context, "unloadMovieNum", strlen("unloadMovieNum"), unload_numbered_builtin, navigation, 0);
    }
    return status;
}


/* [GC:] Notify only while the owner holds its ordinary root/input pins and no VM frame is active.
 * The copied snapshot never escapes; the callback owns its independent userdata. */
static FVM_Result playback_observe(FVM_DesktopPlaybackSession *control,
                                   FVM_DesktopPlaybackPhase phase,
                                   FVM_Context *context,
                                   FVM_Object *root,
                                   const char *source,
                                   unsigned long ticks,
                                   unsigned long presentations)
{
    FVM_DesktopPlaybackState state;
    FVM_DesktopPlaybackCommand command;
    if (!control || !control->policy.notify || control->stopped)
    {
        return FVM_OK;
    }
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.phase = phase;
    if (!control->clock_started)
    {
        control->start_ms = fvm_desktop_now();
        control->clock_started = 1;
    }
    state.elapsed_ms = fvm_desktop_now() - control->start_ms;
    state.context = context;
    state.root = root;
    state.source = source;
    state.movie_ticks = ticks;
    state.total_ticks = control->total_ticks;
    state.presentations = presentations;
    state.paused = control->paused;
    state.step_pending = control->step_pending;
    command = control->policy.notify(&state, control->policy.userdata);
    return fvm_desktop_playback_apply(control, command) ? FVM_OK : FVM_INVALID_ARGUMENT;
}


/* Poll the native error latch without resuming the PCM producer while timeline control is paused.
 */
static FVM_Result playback_audio(FVM_Context *context,
                                 FVM_Object *clip,
                                 FVM_DesktopAudio *audio,
                                 const FVM_DesktopPlaybackSession *control)
{
    FVM_Result result = fvm_desktop_audio_timeline(context, clip, audio);
    if (result != FVM_OK)
    {
        return result;
    }
    return fvm_desktop_audio_tick_paused(audio, control && control->paused);
}
