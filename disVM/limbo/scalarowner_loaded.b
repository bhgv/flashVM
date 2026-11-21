# Copyright (c) 2026 bhgv
# EN: Publish verified exports from one shared original Dis data instance.
# RU: Публикация проверенных экспортов одного исходного экземпляра данных Dis.

Scalarloadedview: type (int, Scalarimplementation, Scalarbigimplementation, Scalartupleimplementation,
    Scalarrefimplementation, Scalarlistimplementation, Scalarrefpairimplementation);


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
    run_loaded(routes, routes->scalarloaded(implementation, name, offer), request, reply);
}


# EN: Invoke the typed slot selected from the actual compiled export.
# RU: Вызывать типизированный слот фактически связанного экспорта.
run_loaded(routes: Vfsnative, view: Scalarloadedview, request, reply: string)
{
    (tag, narrow, wide, pair, single, arguments, aliases) := view;
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
        view := routes->scalarloadedview(anchor, name, offer);
        (tag, nil, nil, nil, nil, nil, nil) := view;
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
