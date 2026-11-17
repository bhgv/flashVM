/* Copyright (c) 2026 bhgv
 * EN: Shared player resource_service implementation; included in owner-thread dependency order.
 * RU: Shared player resource_service; подключается в порядке owner-thread зависимостей.
 */


/* EN: Drain bounded requests after callbacks; context/navigation are owner-thread borrowed.
 * Native staged bytes are released before return; status reports actual service/import failure.
 * RU: Обработать очередь после callbacks; context/navigation borrowed в owner thread.
 * Временные bytes освобождаются до return; вернуть фактический service/import status. */
static FVM_Result service_requests(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_Result unloaded = service_unloads(context, navigation);
    if (unloaded != FVM_OK)
    {
        return unloaded;
    }
    /* A pin retains storage, not membership: removed ancestors invalidate an active target. */
    if (navigation->active_request)
    {
        int attached = 0;
        FVM_Result status =
            fvm_movieclip_attached_to(context, navigation->active_request->target, navigation->root, &attached);
        if (status != FVM_OK)
        {
            return status;
        }
        if (!attached)
        {
            abandon_active(context, navigation);
        }
    }
    /* [GC:] The active request retains its explicit target root while the VM keeps running. */
    if (navigation->active_request)
    {
        FVM_DesktopTransferView view;
        FVM_DesktopRequest *request = navigation->active_request;
        unsigned int kind =
            request->flags & FVM_AVM1_URL_LOAD_VARIABLES ? FVM_DESKTOP_LOAD_VARIABLES : FVM_DESKTOP_LOAD_CHILD;
        FVM_Result status;
        memset(&view, 0, sizeof(view));
        status = background_poll(navigation->transfer, &view);
        if (status == FVM_OK && view.state == FVM_DESKTOP_TRANSFER_PENDING)
        {
            background_notice(navigation->background,
                              navigation->transfer_id,
                              kind,
                              FVM_DESKTOP_LOAD_WAIT,
                              request->url,
                              request->base,
                              &view,
                              FVM_NOT_FOUND);
            return FVM_OK; /* Host stop is handled by owner loop then joined cleanup. */
        }
        if (status == FVM_OK)
        {
            status = view.result;
        }
        background_terminal(
            navigation->background, navigation->transfer_id, kind, request->url, request->base, &view, status, 0);
        if (status == FVM_OK)
        {
            if (request->flags & FVM_AVM1_URL_LOAD_VARIABLES)
            {
                status = fvm_object_load_variables(context, request->target, view.bytes, view.size);
            }
            else
            {
                status = fvm_movieclip_replace_resource(
                    context, request->target, view.bytes, view.size, view.location, strlen(view.location));
            }
        }
        fvm_desktop_transfer_release(navigation->transfer);
        navigation->transfer = 0;
        navigation->active_request = 0;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    while (navigation->head && !navigation->pending)
    {
        FVM_DesktopRequest *request = navigation->head;
        FVM_DesktopDownload download;
        unsigned char *file_bytes = 0;
        const void *bytes = 0;
        size_t size = 0;
        char resolved[FVM_DESKTOP_MAX_LOCATION + 1];
        int variables, attached = 0;
        FVM_Result status = service_unloads(context, navigation);
        if (status != FVM_OK)
        {
            return status;
        }
        if (!navigation->head)
        {
            break;
        }
        request = navigation->head;
        variables = (request->flags & FVM_AVM1_URL_LOAD_VARIABLES) != 0;
        navigation->head = request->next;
        if (!navigation->head)
        {
            navigation->tail = 0;
        }
        --navigation->count;
        status = fvm_movieclip_attached_to(context, request->target, navigation->root, &attached);
        if (status != FVM_OK || !attached)
        {
            release_request(context, request);
            if (status != FVM_OK)
            {
                return status;
            }
            continue; /* Detached storage is not a resource destination and consumes no budget. */
        }
        if (!navigation->remaining)
        {
            status = FVM_UNSUPPORTED;
        }
        else
        {
            --navigation->remaining;
        }
        /* Whole-root replacement retains its textual request beyond old-context cleanup. */
        if (status == FVM_OK && !variables && request->target == navigation->root)
        {
            navigation->pending = request->url;
            navigation->pending_base = request->base;
            request->url = request->base = 0;
        }
        else if (status == FVM_OK && navigation->background && navigation->background->asynchronous &&
                 (strstr(request->url, "://") || strstr(request->base, "://")))
        {
            status = background_begin(navigation->background,
                                      request->url,
                                      request->base,
                                      variables ? FVM_DESKTOP_LOAD_VARIABLES : FVM_DESKTOP_LOAD_CHILD,
                                      &navigation->transfer,
                                      &navigation->transfer_id);
            if (status == FVM_OK)
            {
                navigation->active_request = request;
                return FVM_OK; /* One FIFO resource; owner loop continues input/timers/timeline/GC.
                                */
            }
        }
        else if (status == FVM_OK)
        {
            memset(&download, 0, sizeof(download));
            download.status = FVM_OK;
            memset(resolved, 0, sizeof(resolved));
            if (navigation->managed || strstr(request->url, "://") || strstr(request->base, "://"))
            {
                if (navigation->background)
                {
                    navigation->background->kind = variables ? FVM_DESKTOP_LOAD_VARIABLES : FVM_DESKTOP_LOAD_CHILD;
                }
                if (navigation->managed)
                {
                    status = navigation->managed->fetch(
                        variables ? FVM_DESKTOP_FETCH_DATA : FVM_DESKTOP_FETCH_CODE,
                        request->url, request->base, collect_movie, &download, resolved,
                        sizeof(resolved), navigation->managed->userdata);
                }
                else
                {
                    status = navigation->policy->fetch ? navigation->policy->fetch(
                        request->url, request->base, collect_movie, &download, resolved,
                        sizeof(resolved), navigation->policy->userdata) : FVM_UNSUPPORTED;
                }
                if (download.status != FVM_OK)
                {
                    status = download.status;
                }
                if (!memchr(resolved, 0, sizeof(resolved)) || !location_size(resolved))
                {
                    status = FVM_INVALID_ARGUMENT;
                }
                bytes = download.bytes;
                size = download.size;
            }
            else if (!local_location(request->url, request->base, resolved))
            {
                status = FVM_INVALID_ARGUMENT;
            }
            else
            {
                file_bytes = read_resource(resolved, &size, variables);
                if (!file_bytes)
                {
                    status = FVM_ERROR;
                }
                bytes = file_bytes;
            }
            if (navigation->background && navigation->background->closed)
            {
                status = FVM_OK;
            }
            else if (status == FVM_OK)
            {
                if (variables)
                {
                    status = fvm_object_load_variables(context, request->target, bytes, size);
                }
                else
                {
                    status = fvm_movieclip_replace_resource(
                        context, request->target, bytes, size, resolved, strlen(resolved));
                }
            }
            free(file_bytes);
            free(download.bytes);
        }
        /* [GC:] User setters have returned; drop only this queue entry's explicit root. */
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
        if (navigation->background && navigation->background->closed)
        {
            return FVM_OK;
        }
        if (status != FVM_OK)
        {
            return status;
        }
    }
    return FVM_OK;
}
