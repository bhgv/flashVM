# Copyright (c) 2026 bhgv
# EN: Publish verified exports from one shared original Dis data instance.
# RU: Публикация проверенных экспортов одного исходного экземпляра данных Dis.

Scalarloadedview: type (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
    Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation, Scalarrawinvoke, int, int, Scalarmanagedinvoke);


# EN: Load one export, publish its qualified wire profile and dispatch its typed view.
# RU: Загрузить экспорт, опубликовать проверенный профиль и использовать его тип.
serve_loaded(routes: Vfsnative, fields: list of string)
{
    implementation := hd fields;
    fields = tl fields;
    name := hd fields;
    fields = tl fields;
    request := hd fields;
    fields = tl fields;
    reply := hd fields;
    offer := hd tl fields;
    anchor := routes->scalarloadanchor(implementation);
    pool := new_call_pool();
    run_loaded(routes, routes->scalarloadedslot(anchor, name, offer), request, reply, pool);
    close_call_pool(pool);
}


# EN: Invoke the typed slot selected from the actual compiled export.
# RU: Вызывать типизированный слот фактически связанного экспорта.
run_loaded(routes: Vfsnative, view: Scalarloadedview, request, reply: string, pool: Callpool)
{
    (tag, narrow, wide, pair, single, arguments, aliases, raw, inputs, outputs, managed) := view;
    if (tag == 0) {
        return;
    }
    if (narrow != nil) {
        run_int(routes, narrow, request, reply, big tag, big 73, 0);
    } else if (wide != nil) {
        run_big(routes, wide, request, reply, big tag, big 73);
    } else if (pair != nil) {
        run_tuple(routes, pair, request, reply);
    } else if (single != nil) {
        run_ref(routes, single, request, reply, outputs, pool);
    } else if (arguments != nil) {
        run_list(routes, arguments, request, reply);
    } else if (aliases != nil) {
        run_ref_pair(routes, aliases, request, reply, outputs, pool);
    } else if (managed != nil) {
        for (;;) {
            wire := routes->scalarmanagednext(request);
            if (wire == nil) {
                return;
            }
            submit_call(routes, pool, ref Calljob(reply, 0, 0, (big 0,big 0,big 0,big 0),
                nil, nil, nil, managed, wire));
        }
    } else if (raw != nil) {
        run_raw(routes, raw, request, reply, inputs, outputs, pool);
    }
}



# EN: Drain one verified inbox, then acknowledge its EOF to the owner process.
# RU: Обслуживать проверенный приёмник, затем подтвердить EOF владельцу.
serve_loaded_view(routes: Vfsnative, view: Scalarloadedview, request, reply: string, pool: Callpool, done: chan of int)
{
    run_loaded(routes, view, request, reply, pool);
    done <-= 1;
}


# EN: Stage all typed views before dispatch, then retain them through client shutdown.
# RU: Связать все типизированные представления до вызовов и сохранить до завершения.
serve_loaded_set(routes: Vfsnative, count: int, fields: list of string, table: int)
{
    stride := 4;
    implementation := "";
    if (table) {
        stride = 5;
    } else {
        if (count < 5) {
            raise "missing loaded owner implementation";
        }
        implementation = hd fields;
        fields = tl fields;
        count--;
    }
    limit := 8;
    if (table) {
        limit = 64;
    }
    if (count < stride || count > limit*stride || count%stride != 0) {
        raise "invalid loaded owner export set";
    }
    anchors: list of (string, Scalarloadedanchor);
    planned: list of (Scalarloadedview, string, string);
    for (i := 0; i < count; i += stride) {
        if (table) {
            implementation = hd fields;
            fields = tl fields;
        }
        anchor := find_loaded_anchor(anchors, implementation);
        if (anchor == nil) {
            anchor = routes->scalarloadanchor(implementation);
            anchors = (implementation, anchor) :: anchors;
        }
        name := hd fields;
        fields = tl fields;
        request := hd fields;
        fields = tl fields;
        reply := hd fields;
        fields = tl fields;
        offer := hd fields;
        fields = tl fields;
        view := routes->scalarloadedslot(anchor, name, offer);
        (tag, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil) := view;
        if (tag == 0) {
            if (table) {
                raise "cannot publish loaded export " + implementation + " " + name;
            }
            return;
        }
        planned = (view, request, reply) :: planned;
    }
    serve_live_views(routes, planned, count/stride, table == 2);
}


# EN: Publish and serve every verified export of one live load.
# RU: Опубликовать и обслуживать все проверенные экспорты одной живой загрузки.
serve_live_loaded(sys: Sys, routes: Vfsnative, path: string)
{
    anchor := routes->scalarloadanchor(path);
    profile := routes->scalarloadedprofile(anchor);
    if (profile == nil || !routes->scalarliveprepare(profile)) {
        return;
    }
    (count, names) := sys->tokenize(routes->scalarloadedexports(anchor), "\t");
    planned: list of (Scalarloadedview, string, string);
    for (i := 0; names != nil; i++) {
        suffix := string i;
        view := routes->scalarloadedslot(anchor, hd names, "/.dis-owner/offer" + suffix);
        (tag, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil) := view;
        if (!tag) {
            return;
        }
        planned = (view, "/.dis-owner/request" + suffix, "/.dis-owner/reply" + suffix) :: planned;
        names = tl names;
    }
    if (!routes->scalarliveprepare(nil)) {
        return;
    }
    serve_live_views(routes, planned, count, 0);
}


# EN: Dispatch the prepared views and wait for every exact inbox to retire.
# RU: Запустить подготовленные представления и дождаться закрытия каждой очереди.
serve_live_views(routes: Vfsnative, planned: list of (Scalarloadedview, string, string), count, listen: int)
{
    done := chan of int;
    pool := new_call_pool();
    for (pending := planned; pending != nil; pending = tl pending) {
        (view, request, reply) := hd pending;
        spawn serve_loaded_view(routes, view, request, reply, pool, done);
    }
    if (listen) {
        spawn accept_shared_launch(routes);
    }
    for (i := 0; i < count; i++) {
        <-done;
    }
    close_call_pool(pool);
}



# EN: Reuse the retained data instance for an implementation path.
# RU: Переиспользовать удерживаемый экземпляр данных по пути реализации.
find_loaded_anchor(anchors: list of (string, Scalarloadedanchor), implementation: string): Scalarloadedanchor
{
    for (; anchors != nil; anchors = tl anchors) {
        (path, anchor) := hd anchors;
        if (path == implementation) {
            return anchor;
        }
    }
    return nil;
}


# EN: Invoke the single validated raw slot for this flattened argument count.
# RU: Вызвать единственный проверенный слот для этого количества скалярных ячеек.
invoke_raw(target: Scalarrawinvoke, inputs: int, a,b,c,d: big): (big,big,big,big)
{
    case inputs {
    0 => return target->call0();
    1 => return target->call1(a);
    2 => return target->call2(a, b);
    3 => return target->call3(a, b, c);
    4 => return target->call4(a, b, c, d);
    }
    raise "invalid dynamic call slot";
}


# EN: Receive raw arguments and submit scoped calls to the module dispatcher.
# RU: Принять аргументы и передать контекстные вызовы диспетчеру модуля.
run_raw(routes: Vfsnative, target: Scalarrawinvoke, request, reply: string, inputs, outputs: int, pool: Callpool)
{
    for (;;) {
        (ready, a, b, c, d) := routes->scalarrawdispatchnext(request);
        if (ready == 0) {
            return;
        }
        if (ready != 2) {
            finish_raw(routes, target, reply, inputs, outputs, ready, a, b, c, d);
        } else {
            submit_call(routes, pool, ref Calljob(reply, outputs, inputs, (a,b,c,d), target, nil, nil, nil, nil));
        }
    }
}


# EN: Invoke one admitted call and send its response through the worker's scope.
# RU: Выполнить один принятый вызов и отправить ответ через контекст исполнителя.
finish_raw(routes: Vfsnative, target: Scalarrawinvoke, reply: string, inputs, outputs, ready: int, a,b,c,d: big): int
{
    result := (big 0, big 0, big 0, big 0);
    success := 0;
    if (ready > 0) {
        {
            if (ready == 2) {
                routes->scalarcallstart();
            }
            result = invoke_raw(target, inputs, a, b, c, d);
            success = 1;
        } exception {
            "*" =>
                success = 0;
        }
    }
    cancelled := routes->callcancelled();
    routes->scalarrawreply(reply, success, outputs, result);
    return cancelled;
}


# EN: Dispatch one-ref snapshots until EOF, preserving the unscoped call path.
# RU: Раздавать снимки ref до EOF, сохраняя вызовы без контекста.
run_ref(routes: Vfsnative, target: Scalarrefimplementation, request, reply: string, outputs: int, pool: Callpool)
{
    for (;;) {
        (ready, narrow, wide) := routes->scalarrefdispatchnext(request);
        if (ready == 0) {
            return;
        }
        if (ready == 2) {
            submit_call(routes, pool, ref Calljob(reply, outputs, 1, (big narrow,wide,big 0,big 0), nil, target, nil, nil, nil));
        } else {
            finish_ref(routes, target, reply, outputs, ready, narrow, wide);
        }
    }
}


# EN: Call the linked one-ref export and publish its updated snapshot.
# RU: Вызвать связанный экспорт ref и отправить изменённый снимок.
finish_ref(routes: Vfsnative, target: Scalarrefimplementation, reply: string, outputs, ready, narrow: int, wide: big): int
{
    if (ready < 0) {
        routes->scalarrefreply(reply, 0, (0, big 0), (0, big 0));
        return 0;
    }
    cell := ref Scalarrefcell(narrow, wide);
    result := (0, big 0);
    success := 1;
    {
        if (ready == 2) {
            routes->scalarcallstart();
        }
        result = target->invoke(cell);
    } exception {
        "*" =>
            success = 0;
    }
    cancelled := routes->callcancelled();
    routes->scalarrefreply(reply, success, ref_result(result, outputs), (cell.narrow, cell.wide));
    return cancelled;
}


# EN: Dispatch one or two unique ref snapshots until EOF. Counts above two
# carry the worker handoff marker, removed before reconstructing aliases.
# RU: Раздавать один или два снимка ref до EOF. Счётчик выше двух включает
# признак передачи исполнителю, который снимается перед восстановлением алиасов.
run_ref_pair(routes: Vfsnative, target: Scalarrefpairimplementation, request, reply: string, outputs: int, pool: Callpool)
{
    for (;;) {
        (count, firstn, firstw, secondn, secondw) := routes->scalarrefpairdispatchnext(request);
        if (count == 0) {
            return;
        }
        if (count > 2) {
            submit_call(routes, pool, ref Calljob(reply, outputs, count-2, (big firstn,firstw,big secondn,secondw), nil, nil, target, nil, nil));
        } else {
            finish_ref_pair(routes, target, reply, outputs, count, 0, firstn, firstw, secondn, secondw);
        }
    }
}


# EN: Reconstruct ref aliases in the executing Prog and return successful updates.
# RU: Восстановить алиасы ref в выполняющем Prog и отправить успешные изменения.
finish_ref_pair(routes: Vfsnative, target: Scalarrefpairimplementation, reply: string, outputs, count, worker, firstn: int, firstw: big, secondn: int, secondw: big): int
{
    if (count < 0) {
        routes->scalarrefpairreply(reply, 0, 0, (0, big 0), (0, big 0), (0, big 0));
        return 0;
    }
    first := ref Scalarrefcell(firstn, firstw);
    second := first;
    if (count == 2) {
        second = ref Scalarrefcell(secondn, secondw);
    }
    result := (0, big 0);
    success := 1;
    {
        if (worker) {
            routes->scalarcallstart();
        }
        result = target->invoke(first, second);
    } exception {
        "*" =>
            success = 0;
    }
    cancelled := routes->callcancelled();
    routes->scalarrefpairreply(reply, success, count, ref_result(result, outputs),
        (first.narrow, first.wide), (second.narrow, second.wide));
    return cancelled;
}

include "scalarowner_pool.b";

# EN: Accept an optional hosted launch while the export workers serve the table.
# RU: Принять опциональный запуск параллельно обслуживанию таблицы экспортов.
accept_shared_launch(routes: Vfsnative)
{
    routes->acceptsharedlaunch();
}
