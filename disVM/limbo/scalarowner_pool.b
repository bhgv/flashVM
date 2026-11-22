# Copyright (c) 2026 bhgv
# EN: Share scoped call executors across exports of one loaded Dis instance.
# RU: Совместно использовать исполнителей вызовов экспортами одного экземпляра Dis.

Callkey: type (big,big,big,big,int);
Callargs: type (big,big,big,big);

# EN: One validated invocation, with exactly one linked export target.
# RU: Один проверенный вызов с единственной целью связанного экспорта.
Calljob: adt {
    reply: string;
    outputs, inputs: int;
    args: Callargs;
    raw: Scalarrawinvoke;
    single: Scalarrefimplementation;
    pair: Scalarrefpairimplementation;
    managed: Scalarmanagedinvoke;
    wire: array of byte;
};

# EN: Per-caller state. Receiving its gate grants access to PID and pending status.
# RU: Состояние клиента. Получение gate даёт доступ к PID и состоянию вызова.
Callworker: adt {
    key: Callkey;
    pid, pending: int;
    gate, completed, claimed: chan of int;
    jobs: chan of (int, ref Calljob);
};

Callpool: type chan of list of ref Callworker;


# EN: Return a pool shared by dispatchers of the same loaded instance.
# RU: Вернуть пул диспетчеров одного загруженного экземпляра.
new_call_pool(): Callpool
{
    pool := chan[1] of list of ref Callworker;
    pool <-= nil;
    return pool;
}


# EN: Compare all cells of two native-validated caller identities.
# RU: Сравнить все ячейки двух проверенных идентификаторов клиента.
same_call_key(left, right: Callkey): int
{
    (a,b,c,d,p) := left;
    (e,f,g,h,q) := right;
    return a == e && b == f && c == g && d == h && p == q;
}


# EN: Find or insert a caller row under the pool channel's ownership.
# RU: Найти или добавить запись клиента под владением канала пула.
find_call_worker(pool: Callpool, key: Callkey): ref Callworker
{
    workers := <-pool;
    worker: ref Callworker;
    for (remaining := workers; remaining != nil; remaining = tl remaining) {
        current := hd remaining;
        if (same_call_key(current.key, key)) {
            worker = current;
            break;
        }
    }
    if (worker == nil) {
        worker = ref Callworker(key, 0, 0, chan[1] of int, chan[1] of int,
            chan of int, chan of (int, ref Calljob));
        worker.gate <-= 1;
        workers = worker :: workers;
    }
    pool <-= workers;
    return worker;
}


# EN: Wait only for this caller's previous invocation and hand off the new one.
# RU: Дождаться предыдущего вызова этого клиента и передать новый вызов.
submit_call(routes: Vfsnative, pool: Callpool, job: ref Calljob)
{
    sys := load Sys Sys->PATH;
    worker := find_call_worker(pool, routes->scalarcallkey());
    <-worker.gate;
    {
        if (worker.pending) {
            worker.pending = 0;
            if (<-worker.completed == 0) {
                worker.pid = 0;
            }
        }
        source := 0;
        if (worker.pid == 0) {
            started := chan[1] of int;
            spawn call_worker(sys, routes, worker.jobs, worker.completed, worker.claimed, started);
            worker.pid = <-started;
        } else {
            source = sys->pctl(0, nil);
            routes->scalarcallstage(worker.pid);
        }
        worker.jobs <-= (source, job);
        if (<-worker.claimed != 0) {
            worker.pending = 1;
        } else {
            worker.pid = 0;
            routes->scalarcallabort();
        }
    } exception {
        "*" =>
            routes->scalarcallabort();
    }
    worker.gate <-= 1;
}


# EN: Invoke the job's verified slot in the current caller's Prog.
# RU: Вызвать проверенный слот задания в Prog текущего клиента.
invoke_call_job(routes: Vfsnative, job: ref Calljob): int
{
    if (job.managed != nil) {
        {
            routes->scalarcallstart();
            job.managed->invoke(job.wire);
            return 0;
        } exception {
            "*" =>
                cancelled := routes->callcancelled();
                routes->scalarmanagederror();
                return cancelled;
        }
    }
    (a,b,c,d) := job.args;
    if (job.raw != nil) {
        return finish_raw(routes, job.raw, job.reply, job.inputs, job.outputs, 2, a, b, c, d);
    }
    if (job.single != nil) {
        return finish_ref(routes, job.single, job.reply, job.outputs, 2, int a, b);
    }
    return finish_ref_pair(routes, job.pair, job.reply, job.outputs, job.inputs, 1, int a, b, int c, d);
}


# EN: Keep one executor over successful calls and retire it after cancellation.
# RU: Сохранять исполнителя между успешными вызовами и завершить после отмены.
call_worker(sys: Sys, routes: Vfsnative, jobs: chan of (int, ref Calljob), completed, claimed, started: chan of int)
{
    started <-= sys->pctl(0, nil);
    for (;;) {
        (source, job) := <-jobs;
        if (job == nil) {
            return;
        }
        accepted := 1;
        {
            if (source != 0) {
                routes->scalarcalltakefrom(source);
            }
        } exception {
            "*" =>
                accepted = 0;
        }
        claimed <-= accepted;
        if (!accepted) {
            return;
        }
        reusable := 0;
        {
            reusable = !invoke_call_job(routes, job);
        } exception {
            "*" =>
                reusable = 0;
        }
        completed <-= reusable;
        if (!reusable) {
            return;
        }
    }
}


# EN: Stop idle workers after all export inboxes have closed.
# RU: Остановить исполнителей после закрытия всех приёмников экспортов.
close_call_pool(pool: Callpool)
{
    for (remaining := <-pool; remaining != nil; remaining = tl remaining) {
        worker := hd remaining;
        if (worker.pid != 0 && (!worker.pending || <-worker.completed != 0)) {
            worker.jobs <-= (0, nil);
        }
    }
}
