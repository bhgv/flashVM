# Copyright (c) 2026 bhgv
# EN: Publish verified exports from one shared original Dis data instance.
# RU: Публикация проверенных экспортов одного исходного экземпляра данных Dis.

Scalarloadedview: type (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
    Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation, Scalarrawinvoke, int, int);


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
    run_loaded(routes, routes->scalarloadedslot(anchor, name, offer), request, reply);
}


# EN: Invoke the typed slot selected from the actual compiled export.
# RU: Вызывать типизированный слот фактически связанного экспорта.
run_loaded(routes: Vfsnative, view: Scalarloadedview, request, reply: string)
{
    (tag, narrow, wide, pair, single, arguments, aliases, raw, inputs, outputs) := view;
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
        run_ref(routes, single, request, reply, outputs);
    } else if (arguments != nil) {
        run_list(routes, arguments, request, reply);
    } else if (aliases != nil) {
        run_ref_pair(routes, aliases, request, reply, outputs);
    } else if (raw != nil) {
        run_raw(routes, raw, request, reply, inputs, outputs);
    }
}



# EN: Drain one verified inbox, then acknowledge its EOF to the owner process.
# RU: Обслуживать проверенный приёмник, затем подтвердить EOF владельцу.
serve_loaded_view(routes: Vfsnative, view: Scalarloadedview, request, reply: string, done: chan of int)
{
    run_loaded(routes, view, request, reply);
    done <-= 1;
}


# EN: Stage all typed views before dispatch, then retain them through client shutdown.
# RU: Связать все типизированные представления до вызовов и сохранить до завершения.
serve_loaded_set(routes: Vfsnative, count: int, fields: list of string)
{
    if (count < 5 || count > 33 || (count-1)%4 != 0) {
        raise "invalid loaded owner export set";
    }
    anchor := routes->scalarloadanchor(hd fields);
    fields = tl fields;
    planned: list of (Scalarloadedview, string, string);
    for (i := 1; i < count; i += 4) {
        name := hd fields;
        fields = tl fields;
        request := hd fields;
        fields = tl fields;
        reply := hd fields;
        fields = tl fields;
        offer := hd fields;
        fields = tl fields;
        view := routes->scalarloadedslot(anchor, name, offer);
        (tag, nil, nil, nil, nil, nil, nil, nil, nil, nil) := view;
        if (tag == 0) {
            return;
        }
        planned = (view, request, reply) :: planned;
    }
    done := chan of int;
    for (pending := planned; pending != nil; pending = tl pending) {
        (view, request, reply) := hd pending;
        spawn serve_loaded_view(routes, view, request, reply, done);
    }
    for (i = 1; i < count; i += 4) {
        <-done;
    }
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


Rawkey: type (big,big,big,big,int);
Rawargs: type (big,big,big,big);

Rawworker: adt {
    key: Rawkey;
    pid: int;
    jobs: chan of (int, Rawargs);
    completed: chan of int;
    claimed: chan of int;
};


# EN: Compare every cell of the checked session and caller identity.
# RU: Сравнить все ячейки проверенных идентификаторов сессии и клиента.
same_raw_key(left, right: Rawkey): int
{
    (a,b,c,d,p) := left;
    (e,f,g,h,q) := right;
    return a == e && b == f && c == g && d == h && p == q;
}


# EN: Dispatch each scoped call to the worker for that session and caller PID.
# RU: Передать каждый вызов исполнителю с ключом сессии и PID клиента.
run_raw(routes: Vfsnative, target: Scalarrawinvoke, request, reply: string, inputs, outputs: int)
{
    sys := load Sys Sys->PATH;
    workers: list of ref Rawworker;
    for (;;) {
        (ready, a, b, c, d) := routes->scalarrawdispatchnext(request);
        if (ready == 0) {
            break;
        }
        if (ready != 2) {
            finish_raw(routes, target, reply, inputs, outputs, ready, a, b, c, d);
            continue;
        }
        key := routes->scalarcallkey();
        worker: ref Rawworker;
        for (remaining := workers; remaining != nil; remaining = tl remaining) {
            current := hd remaining;
            if (same_raw_key(current.key, key)) {
                worker = current;
                break;
            }
        }
        {
            if (worker == nil) {
                worker = ref Rawworker(key, 0, chan of (int, Rawargs), chan[1] of int, chan of int);
                workers = worker :: workers;
            }
            if (worker.pid != 0 && <-worker.completed == 0) {
                worker.pid = 0;
            }
            if (worker.pid == 0) {
                started := chan[1] of int;
                spawn raw_worker(sys, routes, target, reply, inputs, outputs, worker.jobs, worker.completed,
                    worker.claimed, started, (a,b,c,d));
                worker.pid = <-started;
            } else {
                routes->scalarcallstage(worker.pid);
                worker.jobs <-= (1, (a,b,c,d));
                if (<-worker.claimed == 0) {
                    routes->scalarcallabort();
                }
            }
        } exception {
            "*" =>
                routes->scalarcallabort();
        }
    }
    for (remaining := workers; remaining != nil; remaining = tl remaining) {
        worker := hd remaining;
        if (worker.pid != 0 && <-worker.completed != 0) {
            worker.jobs <-= (0, (big 0,big 0,big 0,big 0));
        }
    }
}


# EN: Keep an idle worker until its dispatcher shuts down or the call is revoked.
# RU: Сохранять исполнителя в ожидании до остановки диспетчера или отмены вызова.
raw_worker(sys: Sys, routes: Vfsnative, target: Scalarrawinvoke, reply: string, inputs, outputs: int,
    jobs: chan of (int, Rawargs), completed, claimed, started: chan of int, args: Rawargs)
{
    started <-= sys->pctl(0, nil);
    for (;;) {
        reusable := 0;
        {
            (a,b,c,d) := args;
            reusable = !finish_raw(routes, target, reply, inputs, outputs, 2, a, b, c, d);
        } exception {
            "*" =>
                reusable = 0;
        }
        completed <-= reusable;
        if (!reusable) {
            return;
        }
        (continuecall, next) := <-jobs;
        if (!continuecall) {
            return;
        }
        accepted := 1;
        {
            routes->scalarcalltake();
        } exception {
            "*" =>
                accepted = 0;
        }
        claimed <-= accepted;
        if (!accepted) {
            completed <-= 0;
            return;
        }
        args = next;
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
run_ref(routes: Vfsnative, target: Scalarrefimplementation, request, reply: string, outputs: int)
{
    for (;;) {
        (ready, narrow, wide) := routes->scalarrefdispatchnext(request);
        if (ready == 0) {
            return;
        }
        if (ready == 2) {
            {
                spawn finish_ref(routes, target, reply, outputs, ready, narrow, wide);
            } exception {
                "*" =>
                    routes->scalarcallabort();
            }
        } else {
            finish_ref(routes, target, reply, outputs, ready, narrow, wide);
        }
    }
}


# EN: Call the linked one-ref export and publish its updated snapshot.
# RU: Вызвать связанный экспорт ref и отправить изменённый снимок.
finish_ref(routes: Vfsnative, target: Scalarrefimplementation, reply: string, outputs, ready, narrow: int, wide: big)
{
    if (ready < 0) {
        routes->scalarrefreply(reply, 0, (0, big 0), (0, big 0));
        return;
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
    routes->scalarrefreply(reply, success, ref_result(result, outputs), (cell.narrow, cell.wide));
}


# EN: Dispatch one or two unique ref snapshots until EOF. Counts above two
# carry the worker handoff marker, removed before reconstructing aliases.
# RU: Раздавать один или два снимка ref до EOF. Счётчик выше двух включает
# признак передачи исполнителю, который снимается перед восстановлением алиасов.
run_ref_pair(routes: Vfsnative, target: Scalarrefpairimplementation, request, reply: string, outputs: int)
{
    for (;;) {
        (count, firstn, firstw, secondn, secondw) := routes->scalarrefpairdispatchnext(request);
        if (count == 0) {
            return;
        }
        if (count > 2) {
            {
                spawn finish_ref_pair(routes, target, reply, outputs, count-2, 1, firstn, firstw, secondn, secondw);
            } exception {
                "*" =>
                    routes->scalarcallabort();
            }
        } else {
            finish_ref_pair(routes, target, reply, outputs, count, 0, firstn, firstw, secondn, secondw);
        }
    }
}


# EN: Reconstruct ref aliases in the executing Prog and return successful updates.
# RU: Восстановить алиасы ref в выполняющем Prog и отправить успешные изменения.
finish_ref_pair(routes: Vfsnative, target: Scalarrefpairimplementation, reply: string, outputs, count, worker, firstn: int, firstw: big, secondn: int, secondw: big)
{
    if (count < 0) {
        routes->scalarrefpairreply(reply, 0, 0, (0, big 0), (0, big 0), (0, big 0));
        return;
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
    routes->scalarrefpairreply(reply, success, count, ref_result(result, outputs),
        (first.narrow, first.wide), (second.narrow, second.wide));
}
