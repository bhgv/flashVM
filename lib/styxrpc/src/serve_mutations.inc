/* Copyright (c) 2026 bhgv
 * EN: Opt-in ordinary Tcreate and Tremove on the hosted callback tree.
 * RU: Явное подключение обычных Tcreate и Tremove к дереву колбэков хоста.
 */


/* EN: Check the live directory for write and search access by the attached peer.
 * Borrow server; root is permitted as a parent. Directory metadata grants only
 * the server-side policy bits, so the owner callback may still refuse.
 * RU: Проверить права записи и прохода в действующем каталоге для клиента.
 * server заимствован, корень может быть родителем. Колбэк вправе отказать. */
static int mutable_parent(StyxRpcServer *server, uint64_t id)
{
    StyxRpcNode node;
    if (!lookup(server, id, &node) || !node.entry.directory || (node.entry.mode & 3U) != 3U)
    {
        return 0;
    }
    for (size_t depth = 0; id && depth < STYXRPC_VFS_PATH_MAX; ++depth)
    {
        if (STYXRPC_EXCHANGE_QID(id) || native_program(id) ||
            styxrpc_vfs_service_name(node.entry.name, strlen(node.entry.name)))
        {
            return 0;
        }
        id = node.parent;
        if (!lookup(server, id, &node) || !node.entry.directory)
        {
            return 0;
        }
    }
    return id == 0;
}


/* EN: Create and open one ordinary child on the current directory fid. On
 * rejection leave the directory fid unopened; replace qid only on success.
 * name is borrowed for the callback. Match the native DisVM creation profile.
 * RU: Создать и открыть одного потомка на текущем fid каталога. При отказе
 * fid остаётся неоткрытым, qid меняется только при успехе. Имя заимствовано
 * на время колбэка. Режимы совпадают с профилем создания DisVM. */
static char *create_node(Qid *qid, char *name, int permissions, int mode)
{
    StyxRpcServer *server = active;
    int directory = (unsigned)permissions == ((unsigned)DMDIR | 0777U) && mode == OREAD;
    if (!server->mutations || !server->mutations->create || server->control_only || server->program_only ||
        (!directory && (permissions != 0666 || mode != OWRITE || !server->tree->files.write)) ||
        !name || !*name || strlen(name) > STYXRPC_TREE_NAME || strchr(name, '/') ||
        !strcmp(name, ".") || !strcmp(name, "..") || styxrpc_vfs_service_name(name, strlen(name)) ||
        !mutable_parent(server, qid->path) || styxgetaux(&server->wire))
    {
        return Eperm;
    }
    Qid searched = *qid;
    if (walk_node(&searched, name) != Enonexist)
    {
        return Eperm;
    }
    TreeFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    uint64_t id = 0;
    uint32_t revision = 0;
    void *cookie = NULL;
    if (server->mutations->create(server->context, qid->path, name, directory, &id, &revision,
        directory ? NULL : &cookie) != 1)
    {
        free(fid);
        return Eperm;
    }
    if (!directory)
    {
        fid->file = (StyxRpcFile){.ops = &server->tree->files, .cookie = cookie, .access = STYXRPC_WRITE};
    }
    if (!id || STYXRPC_EXCHANGE_QID(id) || native_program(id) ||
        styxsetauxowned(&server->wire, fid, release_fid) < 0)
    {
        release_fid(fid);
        return Ebadarg;
    }
    *qid = (Qid){.path = id, .vers = revision, .type = directory ? QTDIR : 0};
    return nil;
}


/* EN: Ask the owner to unlink the exact live Qid under a writable parent.
 * The native dispatcher always consumes the requesting fid, including on error.
 * RU: Попросить владельца удалить точный живой Qid в доступном для записи
 * каталоге. Нативный диспетчер поглощает fid запроса даже при отказе. */
static char *remove_node(Qid qid)
{
    StyxRpcServer *server = active;
    StyxRpcNode node;
    if (!server->mutations || !server->mutations->remove || server->control_only || server->program_only ||
        !qid.path || STYXRPC_EXCHANGE_QID(qid.path) || native_program(qid.path) ||
        !lookup(server, qid.path, &node) || styxrpc_vfs_service_name(node.entry.name, strlen(node.entry.name)) ||
        !mutable_parent(server, node.parent))
    {
        return Eperm;
    }
    return server->mutations->remove(server->context, qid.path) == 1 ? nil : Eperm;
}


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_enable_mutations(StyxRpcServer *server, const StyxRpcMutations *mutations)
{
    if (!server || !mutations || (!mutations->create && !mutations->remove) ||
        server->started || server->mutations || server->tree == &styxrpc_control_tree ||
        server->tree == &styxrpc_module_slot_tree || server->tree == &styxrpc_program_tree)
    {
        return 0;
    }
    server->mutations = mutations;
    return 1;
}
