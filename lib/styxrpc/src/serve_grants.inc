/* Copyright (c) 2026 bhgv
 * EN: Serialize temporary grants with dispatch on a program server.
 * RU: Сериализовать выдачу временных ресурсов с диспетчером программ.
 */

/* EN/RU: See program.h. / См. program.h. */
uint32_t styxrpc_program_server_grant(StyxRpcServer *server, const StyxRpcResource *resource)
{
    if (!server || !server->programs)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    uint32_t pid = styxrpc_program_unibus_grant(server->programs, resource);
    pthread_mutex_unlock(&server->program_lock);
    return pid;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_server_revoke(StyxRpcServer *server, uint32_t pid)
{
    if (!server || !server->programs)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    int revoked = styxrpc_program_unibus_revoke(server->programs, pid);
    pthread_mutex_unlock(&server->program_lock);
    return revoked;
}


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
int styxrpc_int_ref_grant(StyxRpcServer *server, const StyxIntVariable *variable, int writable, StyxRpcIntRef *out)
{
    if (!server || !server->programs || !out)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    uint32_t pid = styxrpc_program_int_ref_grant(server->programs, variable, writable);
    pthread_mutex_unlock(&server->program_lock);
    if (!pid)
    {
        return 0;
    }
    *out = (StyxRpcIntRef){pid, writable};
    return 1;
}


/* EN/RU: See variable_ref.h. / См. variable_ref.h. */
int styxrpc_int_ref_revoke(StyxRpcServer *server, const StyxRpcIntRef *ref)
{
    if (!server || !server->programs)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    int revoked = styxrpc_program_int_ref_revoke(server->programs, ref);
    pthread_mutex_unlock(&server->program_lock);
    return revoked;
}
