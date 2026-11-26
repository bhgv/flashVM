/* Copyright (c) 2026 bhgv
 * EN: An owner observer for the ordinary Styx attachment name.
 * RU: Наблюдатель владельца за именем обычного подключения Styx.
 */


/* EN: Report a submitted attach name on the owner without changing admission.
 * RU: Передать владельцу имя запросившего attach без изменения допуска. */
static char *attach_peer(char *user, char *name)
{
    (void)user;
    if (active->attach)
    {
        active->attach(active->attach_state, name);
    }
    return nil;
}


/* EN/RU: See serve.h. / См. serve.h. */
void styxrpc_server_observe_attach(StyxRpcServer *server, StyxRpcAttachNotice notice, void *state)
{
    if (server)
    {
        server->attach = notice;
        server->attach_state = state;
    }
}
