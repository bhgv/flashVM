/* Copyright (c) 2026 bhgv
 * EN: Restrict synchronous reverse progress to the module slot's native program tree.
 * RU: Ограничить встречное обслуживание при синхронном вызове нативным деревом программ.
 */


/* EN: Exclude dynamic callback and image generations from the native program ID range.
 * RU: Исключить поколения образов и колбэков из диапазона нативного дерева. */
static int native_program(uint64_t id)
{
    return id > STYXRPC_SLOT_PROGRAM_ID && id - STYXRPC_SLOT_PROGRAM_ID < STYXRPC_TREE_CHILDREN;
}


/* EN: Close one exact cookie when running in its ordinary owner phase.
 * RU: Закрыть точный курсор в обычной фазе работы владельца. */
static void close_tree_fid(TreeFid *fid)
{
    styxrpc_file_close(&fid->file);
    free(fid);
}


/* EN: Retire deferred closures outside the nested native program turn.
 * RU: Завершить отложенные закрытия вне вложенного нативного такта. */
static void drain_clunks(StyxRpcServer *server)
{
    while (server->deferred)
    {
        TreeFid *fid = server->deferred;
        server->deferred = fid->next;
        close_tree_fid(fid);
    }
}


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_program_step_for(StyxRpcServer *server, int timeout_ms)
{
    if (!server || active || server->tree != &styxrpc_module_slot_tree ||
        !((StyxRpcModuleSlot *)server->context)->programs || server->control_only || timeout_ms < 0)
    {
        return 0;
    }
    server->program_only = 1;
    int ready = styxrpc_server_step_for(server, timeout_ms);
    server->program_only = 0;
    return ready;
}
