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
        run_ref(routes, single, request, reply);
    } else if (arguments != nil) {
        run_list(routes, arguments, request, reply);
    } else if (aliases != nil) {
        run_ref_pair(routes, aliases, request, reply);
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


# EN: Use ordinary Dis calls after marshaling four local scalar input cells.
# RU: Выполнять обычные вызовы Dis после получения локальных скалярных ячеек.
run_raw(routes: Vfsnative, target: Scalarrawinvoke, request, reply: string, inputs, outputs: int)
{
    for (;;) {
        (ready, a, b, c, d) := routes->scalarrawnext(request);
        if (ready == 0) {
            return;
        }
        result := (big 0, big 0, big 0, big 0);
        success := 1;
        {
            result = invoke_raw(target, inputs, a, b, c, d);
        } exception {
            "*" =>
                success = 0;
        }
        routes->scalarrawreply(reply, success, outputs, result);
    }
}
