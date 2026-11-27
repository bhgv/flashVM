/* Copyright (c) 2026 bhgv
 * EN: Restrict file operations to the control node during a stopped host activation.
 * RU: Ограничить файловые операции узлом управления во время остановки хоста.
 */


/* EN/RU: See styxrpc/control.h. / См. styxrpc/control.h. */
int styxrpc_server_step_control(StyxRpcServer *server)
{
    if (!server || active == server ||
        (server->tree != &styxrpc_control_tree && server->tree != &styxrpc_module_slot_tree))
    {
        return 0;
    }
    server->control_only = 1;
    int alive = styxrpc_server_step(server);
    server->control_only = 0;
    return alive;
}
