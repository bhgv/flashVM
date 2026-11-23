/* Copyright (c) 2026 bhgv
 * EN: Private native attachments fixed before publishing an invocation context.
 * RU: Приватные нативные привязки, заданные до публикации контекста вызова.
 */


/* EN/RU: See provider.h. / См. provider.h. */
int dvm_provider_set_call_cancel(DVM_Provider *provider, DVM_Cancel *token)
{
    if (!provider || !token)
    {
        return 0;
    }
    fvm_port_sync_lock(provider->lock);
    int installed = provider->references == 1 && !provider->call_cancel && dvm_cancel_retain(token);
    if (installed)
    {
        provider->call_cancel = token;
    }
    fvm_port_sync_unlock(provider->lock);
    return installed;
}


/* EN/RU: See provider.h. / См. provider.h. */
DVM_Cancel *dvm_provider_call_cancel(DVM_Provider *provider)
{
    if (!provider)
    {
        return NULL;
    }
    fvm_port_sync_lock(provider->lock);
    DVM_Cancel *token = provider->call_cancel;
    if (token && !dvm_cancel_retain(token))
    {
        token = NULL;
    }
    fvm_port_sync_unlock(provider->lock);
    return token;
}


/* EN/RU: See provider.h. / См. provider.h. */
int dvm_provider_set_call_reply(DVM_Provider *context, DVM_Provider *reply)
{
    if (!context || context == reply || !dvm_provider_retain(reply))
    {
        return 0;
    }
    fvm_port_sync_lock(context->lock);
    int installed = context->references == 1 && !context->call_reply;
    if (installed)
    {
        context->call_reply = reply;
    }
    fvm_port_sync_unlock(context->lock);
    if (!installed)
    {
        dvm_provider_release(reply);
    }
    return installed;
}


/* EN/RU: See provider.h. / См. provider.h. */
DVM_Provider *dvm_provider_call_reply(DVM_Provider *context)
{
    if (!context)
    {
        return NULL;
    }
    fvm_port_sync_lock(context->lock);
    DVM_Provider *reply = context->call_reply;
    fvm_port_sync_unlock(context->lock);
    return dvm_provider_retain(reply) ? reply : NULL;
}

/* EN/RU: See provider.h. / См. provider.h. */
int dvm_provider_set_call_export(DVM_Provider *context, DVM_Provider *control)
{
    if (!context || !control || context == control || !dvm_provider_retain(control))
    {
        return 0;
    }
    fvm_port_sync_lock(context->lock);
    int installed = context->references == 1 && !context->call_export;
    if (installed)
    {
        context->call_export = control;
    }
    fvm_port_sync_unlock(context->lock);
    if (!installed)
    {
        dvm_provider_release(control);
    }
    return installed;
}


/* EN/RU: See provider.h. / См. provider.h. */
DVM_Provider *dvm_provider_call_export(DVM_Provider *context)
{
    if (!context)
    {
        return NULL;
    }
    fvm_port_sync_lock(context->lock);
    DVM_Provider *control = context->call_export;
    fvm_port_sync_unlock(context->lock);
    return dvm_provider_retain(control) ? control : NULL;
}
