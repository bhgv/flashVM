/* Copyright (c) 2026 bhgv
 * EN: Public accessors and retirement for the retained Styx server.
 * RU: Доступ к удерживаемому серверу Styx и завершение его работы.
 */

/* EN/RU: Borrow either mailbox of one indexed Dis entry through join.
 * Заимствовать один из приёмников записи Dis по индексу до завершения сервера. */
DVM_Provider *styxunibusservermailbox(StyxUnibusServer *server, size_t index, int reply)
{
    if (!server || index >= server->count)
    {
        return NULL;
    }
    StyxScalarFunctionOwner *owner = &server->modules[index].owner;
    return reply ? owner->reply : owner->request;
}


/* EN/RU: Borrow the first entry's service reply mailbox through join.
 * Заимствовать приёмник ответов первой записи до завершения сервера. */
DVM_Provider *styxunibusserverreply(StyxUnibusServer *server)
{
    return styxunibusservermailbox(server, 0, 1);
}


/* EN/RU: Export only the original file and inbox.
 * Опубликовать исходный файл и приёмник. */
StyxUnibusServer *styxunibusserverstartfile(RemotePhy *channel, unsigned long tag, DVM_Provider *original)
{
    return styxunibusserverstartmodule(channel, tag, original, NULL);
}


/* EN/RU: Preserve the default inbox-only server.
 * Сохранить сервер, публикующий только приёмник. */
StyxUnibusServer *styxunibusserverstart(RemotePhy *channel, unsigned long tag)
{
    return styxunibusserverstartfile(channel, tag, NULL);
}


/* EN/RU: See unibus_server.h; the server retains the original factory.
 * См. unibus_server.h; сервер удерживает исходную фабрику. */
DVM_Provider *styxunibusserverinbox(StyxUnibusServer *server)
{
    return !server ? NULL : server->inbox ? server->inbox : styxunibusservermailbox(server, 0, 0);
}


/* EN/RU: See unibus_server.h; join before releasing the export and factory.
 * См. unibus_server.h; дождаться потока до освобождения экспорта и фабрики. */
int styxunibusserverjoin(StyxUnibusServer *server)
{
    if (!server)
    {
        return 1;
    }
    if (pthread_join(server->thread, NULL))
    {
        return 0;
    }
    styxunibusdrainfunctions(server);
    free_launch_queue(server->launches);
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free_modules(server);
    dvm_provider_release(server->caller_root);
    free(server);
    return 1;
}
