// Copyright (c) 2026 bhgv
// EN: Rendezvous with Limbo on the already attached named channel.
// RU: Обменяться с Limbo в именованном канале уже подключённой сессии.


// EN: Echo the scalar frame and send back an array through the same channel.
// RU: Вернуть скалярное значение и отправить массив через тот же канал.
function onEnterFrame() {
    if (!done) {
        var port = rpc.chan("/jobs");
        var input = port.read();
        var sent = port.write(input);
        var numbers = "3,7";
        var values = numbers.split(",");
        var graphSent = port.write(values);
        var copy = port.read();
        var graphEcho = port.write(copy);
        var retired = port.delete();
        done = true;
    }
}
