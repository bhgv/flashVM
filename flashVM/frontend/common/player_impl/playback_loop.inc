/* Copyright (c) 2026 bhgv
 * EN: Shared player playback loop implementation; included in owner-thread dependency order.
 * RU: Shared player playback loop; подключается в порядке owner-thread зависимостей.
 */
    while (!*quit && (!limit || ticks < limit) && !(navigation && navigation->pending))
    {
        double now;
        if (!fvm_desktop_window_events(window, 0))
        {
            *quit = 1;
            break;
        }
        if (!checked(fvm_desktop_window_status(window), "window events", path))
        {
            goto cleanup;
        }
        {
            int was_paused = control && control->paused;
            if (!checked(
                    playback_observe(control, FVM_DESKTOP_PLAYBACK_TURN, context, clip, path, ticks, presentations),
                    "playback control",
                    path))
            {
                goto cleanup;
            }
            if (control && control->stopped)
            {
                *quit = 1;
                break;
            }
            if (control && (was_paused != control->paused || control->step_pending))
            {
                next = fvm_desktop_now();
            }
        }
        if (!fvm_desktop_window_active(window))
        {
            if (audio.device && !checked(fvm_desktop_audio_pause(&audio, 1), "background audio pause", path))
            {
                goto cleanup;
            }
            if (!fvm_desktop_window_events(window, FVM_DESKTOP_AUDIO_WAIT))
            {
                *quit = 1;
            }
            if (!checked(fvm_desktop_window_status(window), "background window events", path))
            {
                goto cleanup;
            }
            if (*quit)
            {
                break;
            }
            next = fvm_desktop_now();
            continue;
        }
        if (!checked(playback_audio(context, clip, &audio, control), "PCM queue", path))
        {
            goto cleanup;
        }
        if (!checked(fvm_desktop_window_pump_clip_input(window, context, clip, -viewport.x0, -viewport.y0),
                     "input listeners",
                     path))
        {
            goto cleanup;
        }
        fvm_desktop_window_input(window, &input);
        if (!checked(publish_input(context,
                                   host_input,
                                   &input,
                                   (double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL,
                                   (double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL),
                     "host input",
                     path))
        {
            goto cleanup;
        }
        /* Input may register timers; deliver due work even between timeline frames. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
        {
            goto cleanup;
        }
        if (navigation && !checked(service_requests(context, navigation), "deferred resource load", path))
        {
            goto cleanup;
        }
        if (navigation && navigation->background && navigation->background->closed)
        {
            *quit = 1;
            break;
        }
        if (navigation && navigation->pending)
        {
            break;
        }
        /* [GC:] Input listeners and HostInput publication also allocate without timers.
         * Step once per foreground turn after all callbacks return; roots stay pinned.
         * The collector threshold decides whether this safe point actually collects. */
        if (!checked(fvm_gc_step(context, 0), "event GC step", path))
        {
            goto cleanup;
        }
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "cursor visibility", path))
        {
            goto cleanup;
        }
        now = fvm_desktop_now();
        if (!fvm_desktop_playback_can_tick(control))
        {
            int wait;
            if (!checked(fvm_stage_take_update(context, &update_requested), "paused redraw", path))
            {
                goto cleanup;
            }
            if (!presentations || update_requested)
            {
                if (!present_stage(context, clip, window, &surface, &viewport, path))
                {
                    goto cleanup;
                }
                ++presentations;
                ++refreshes;
                if (!frame_observe(frames,
                                   &surface,
                                   path,
                                   ticks,
                                   control ? control->total_ticks : ticks,
                                   presentations,
                                   FVM_DESKTOP_FRAME_REFRESH))
                {
                    goto cleanup;
                }
            }
            if (!checked(fvm_timers_next_delay(context, FVM_DESKTOP_PLAYBACK_WAIT_MS, &timer_wait, &timers_pending),
                         "paused timer wakeup",
                         path))
            {
                goto cleanup;
            }
            wait = (int)timer_wait;
            if (wait && !fvm_desktop_window_events(window, wait))
            {
                *quit = 1;
            }
            if (!checked(fvm_desktop_window_status(window), "paused window events", path))
            {
                goto cleanup;
            }
            next = fvm_desktop_now();
            continue;
        }
        if (now < next && !(control && control->paused && control->step_pending))
        {
            int wait;
            if (!checked(fvm_stage_take_update(context, &update_requested), "redraw request", path))
            {
                goto cleanup;
            }
            if (update_requested)
            {
                if (!present_stage(context, clip, window, &surface, &viewport, path))
                {
                    goto cleanup;
                }
                ++presentations;
                ++refreshes;
                if (!frame_observe(frames,
                                   &surface,
                                   path,
                                   ticks,
                                   control ? control->total_ticks : ticks,
                                   presentations,
                                   FVM_DESKTOP_FRAME_REFRESH))
                {
                    goto cleanup;
                }
                /* A new request made during rendering must not be lost behind a long frame wait. */
                if (!checked(fvm_stage_get_update(context, &update_requested), "redraw state", path))
                {
                    goto cleanup;
                }
                now = fvm_desktop_now();
                if (update_requested || now >= next)
                {
                    continue;
                }
            }
            wait = (int)(next - now);
            if (audio.device && wait > FVM_DESKTOP_AUDIO_WAIT)
            {
                wait = FVM_DESKTOP_AUDIO_WAIT;
            }
            /* Poll owner commands even when a slow, timer-free movie has no SDL events. */
            if (control && control->policy.notify && wait > (int)FVM_DESKTOP_PLAYBACK_WAIT_MS)
            {
                wait = FVM_DESKTOP_PLAYBACK_WAIT_MS;
            }
            if (wait < 1)
            {
                wait = 1;
            }
            /* Do not sleep past a pending deadline when the authored frame rate is low. */
            if (!checked(fvm_timers_next_delay(context, (unsigned long)wait, &timer_wait, &timers_pending),
                         "timer wakeup",
                         path))
            {
                goto cleanup;
            }
            wait = (int)timer_wait;
            if (navigation && navigation->transfer && wait > (int)FVM_DESKTOP_BACKGROUND_WAIT_MS)
            {
                wait = FVM_DESKTOP_BACKGROUND_WAIT_MS; /* Publish completions even at low movie FPS. */
            }
            if (!wait)
            {
                continue; /* A deadline became due after polling; retry without a blocking wait. */
            }
            if (!fvm_desktop_window_events(window, wait))
            {
                *quit = 1;
                break;
            }
            if (!checked(fvm_desktop_window_status(window), "waiting window events", path))
            {
                goto cleanup;
            }
            continue;
        }
        if (!checked(fvm_movieclip_advance(context, clip, 1), "timeline tick", path))
        {
            goto cleanup;
        }
        fvm_desktop_playback_consume_step(control);
        /* Deliver timers on the VM thread before rendering callback-driven scene changes. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
        {
            goto cleanup;
        }
        if (!checked(playback_audio(context, clip, &audio, control), "frame PCM queue", path))
        {
            goto cleanup;
        }
        if (!checked(fvm_stage_take_update(context, &update_requested), "frame redraw request", path))
        {
            goto cleanup;
        }
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "frame cursor visibility", path))
        {
            goto cleanup;
        }
        if (!present_stage(context, clip, window, &surface, &viewport, path))
        {
            goto cleanup;
        }
        ++presentations;
        ++ticks;
        if (control)
        {
            ++control->total_ticks;
        }
        if (!frame_observe(frames,
                           &surface,
                           path,
                           ticks,
                           control ? control->total_ticks : ticks,
                           presentations,
                           FVM_DESKTOP_FRAME_TICK))
        {
            goto cleanup;
        }
        if (navigation && !checked(service_requests(context, navigation), "frame resource load", path))
        {
            goto cleanup;
        }
        if (navigation && navigation->background && navigation->background->closed)
        {
            *quit = 1;
            break;
        }
        /* [GC:] Collect only at a safe host boundary; the root and its library remain pinned. */
        if (!checked(fvm_gc_step(context, 0), "GC step", path))
        {
            goto cleanup;
        }
        if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_TICK, context, clip, path, ticks, presentations),
                     "playback control",
                     path) ||
            (control && !checked(playback_audio(context, clip, &audio, control), "controlled PCM queue", path)))
        {
            goto cleanup;
        }
        if (control && control->stopped)
        {
            *quit = 1;
            break;
        }
        next += period;
        if (now - next > FVM_DESKTOP_LATE_FRAMES * period)
        {
            next = now;
        }
    }
    /* Retain final-frame visibility, then finish authorized stream work within a native deadline. */
    tail_deadline = fvm_desktop_now();
    if (tail_deadline < next)
    {
        tail_deadline = next;
    }
    tail_deadline += FVM_DESKTOP_AUDIO_TAIL_MS;
    while (!*quit && !(navigation && navigation->pending) && !(control && control->paused))
    {
        int wait, pending = 0;
        double tail_now = fvm_desktop_now();
        if (tail_now >= next)
        {
            if (!checked(fvm_desktop_audio_stream_pending(&audio, &pending), "stream tail observation", path))
            {
                goto cleanup;
            }
            if (!pending)
            {
                break;
            }
            if (tail_now >= tail_deadline)
            {
                checked(FVM_ERROR, "stream completion deadline exceeded", path);
                goto cleanup;
            }
        }
        /* Final-frame visibility must not postpone an embedding host's STOP command. */
        if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_TURN, context, clip, path, ticks, presentations),
                     "tail playback control",
                     path))
        {
            goto cleanup;
        }
        if (control && control->stopped)
        {
            *quit = 1;
            break;
        }
        if (control && control->paused)
        {
            break; /* The frame bound already completed; a paused tail need not delay return. */
        }
        wait = tail_now >= next ? FVM_DESKTOP_AUDIO_WAIT : (int)(next - fvm_desktop_now());
        if (control && control->policy.notify && wait > (int)FVM_DESKTOP_PLAYBACK_WAIT_MS)
        {
            wait = FVM_DESKTOP_PLAYBACK_WAIT_MS;
        }
        if (!fvm_desktop_window_active(window))
        {
            if (audio.device && !checked(fvm_desktop_audio_pause(&audio, 1), "tail background audio pause", path))
            {
                goto cleanup;
            }
        }
        else if (!checked(playback_audio(context, clip, &audio, control), "tail PCM queue", path))
        {
            goto cleanup;
        }
        if (audio.device && wait > FVM_DESKTOP_AUDIO_WAIT)
        {
            wait = FVM_DESKTOP_AUDIO_WAIT;
        }
        if (wait < 1)
        {
            wait = 1;
        }
        if (!fvm_desktop_window_events(window, wait))
        {
            *quit = 1;
        }
        if (!checked(fvm_desktop_window_status(window), "final window events", path))
        {
            goto cleanup;
        }
    }
    if (options->stats)
    {
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!checked(fvm_execution_get_stats(context, &stats), "execution statistics", path))
        {
            goto cleanup;
        }
        fprintf(stderr,
                "input_keys=%lu input_clicks=%lu sound_events=%lu sound_stops=%lu audio_device=%d "
                "audio_underruns=%d\n",
                input.key_presses,
                input.clicks,
                audio.events,
                audio.stops,
                audio.device != 0,
                SDL_AtomicGet(&audio.underruns));
        if (audio.composition)
        {
            FVM_SWFStreamMixInfo snapshot;
            FVM_SWFSoundMixQueueOriginInfo origin;
            snapshot.struct_size = sizeof(snapshot);
            if (!checked(fvm_desktop_audio_mix_info(&audio, &snapshot), "mixed clock snapshot", path))
            {
                goto cleanup;
            }
            origin = snapshot.origins;
            fprintf(stderr,
                    "audio_base_submitted=%.0f audio_base_dequeued=%.0f "
                    "audio_tail_submitted=%.0f audio_tail_dequeued=%.0f\n",
                    origin.base_submitted_frames,
                    origin.base_played_frames,
                    origin.tail_submitted_frames,
                    origin.tail_played_frames);
        }
        fprintf(stderr, "presentations=%lu refreshes=%lu\n", presentations, refreshes);
        fprintf(stderr,
                "frames=%lu native=%lu interpreter=%lu fallbacks=%lu\n",
                ticks,
                stats.native_entries,
                stats.interpreter_entries,
                stats.fallbacks);
    }
    status = 0;
