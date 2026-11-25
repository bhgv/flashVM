// Copyright (c) 2026 bhgv
// EN: Exchange a value and a live reference through one named RPC port.
// RU: Обменяться значением и живой ссылкой через один именованный порт RPC.


// EN: Run once against the test provider, which writes a ref before the first read.
// RU: Выполнить один раз с тестовым провайдером, который выдаёт ссылку до первого чтения.
function onEnterFrame() {
    if (!mixedProofDone) {
        var mixed = rpc.newchan("/mixed");
        var mixedCounter = mixed.recv();
        var sent = mixed.send(mixedCounter);
        var forwarded = mixed.send(ref mixedCounter);
        var snapshot = mixed.recv();
        var echoed = mixed.recv();
        echoed = 111;
        _root.proofBefore = snapshot;
        _root.proofAfter = mixedCounter;
        var closed = mixed.close();
        mixedProofDone = true;
    }
}
