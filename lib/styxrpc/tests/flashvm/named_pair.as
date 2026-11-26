// Copyright (c) 2026 bhgv
// EN: Rendezvous with Limbo on the already attached named channel.
// RU: Обменяться с Limbo в именованном канале уже подключённой сессии.


// EN: Echo the scalar frame and send back an array through the same channel.
// RU: Вернуть скалярное значение и отправить массив через тот же канал.
function onEnterFrame() {
    if (!done) {
        var entered = os.cd("/named-area");
        var port = vfs.chan("../jobs");
        var input = port.recv();
        var sent = port.send(input);
        var numbers = "3,7";
        var values = numbers.split(",");
        var graphSent = port.send(values);
        var copy = port.recv();
        var graphEcho = port.send(copy);
        var counter = 21;
        var exported = port.send(ref counter);
        var changed = port.recv();
        var snapshot = port.send(counter);
        counter = true;
        var forwarded = port.send(ref counter);
        var alias = port.recv();
        alias = 111;
        var confirmed = port.send(counter);
        var foreign = port.recv();
        foreign = "edited";
        var returned = port.send(foreign);
        var retired = port.delete();
        done = true;
    }
}
