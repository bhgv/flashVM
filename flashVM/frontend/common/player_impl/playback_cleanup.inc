/* Copyright (c) 2026 bhgv
 * EN: Shared player playback cleanup implementation; included in owner-thread dependency order.
 * RU: Shared player playback cleanup; подключается в порядке owner-thread зависимостей.
 */
cleanup:
    if (navigation && navigation->background)
    {
        navigation->background->window = 0;
        navigation->background->audio = 0;
    }
    if (context)
    {
        discard_requests(context, navigation);
    }
    if (navigation)
    {
        navigation->audio = 0; /* Never retain a pointer to this play_bytes stack frame. */
    }
    /* Release both real-output and explicit-mute registrations before audio userdata cleanup. */
    if (context && clip)
    {
        fvm_movieclip_set_sound_callback(context, 0, 0);
    }
    fvm_desktop_audio_close(&audio);
    fvm_desktop_window_close(window);
    free(surface.pixels);
    /* [GC:] End the host root before destroying its context; no borrowed pointer escapes. */
    if (input_pinned)
    {
        fvm_gc_unpin(context, host_input);
    }
    if (pinned)
    {
        fvm_gc_unpin(context, clip);
    }
    if (context)
    {
        fvm_context_destroy(context);
    }
    if (runtime)
    {
        fvm_runtime_destroy(runtime);
    }
    return status;
