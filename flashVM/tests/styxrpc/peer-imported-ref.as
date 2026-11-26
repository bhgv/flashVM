// Copyright (c) 2026 bhgv
// EN: Use an imported reference in compiled AS.
// RU: Использовать импортированную ссылку из собранного AS.


// EN: Run once using the issuer descriptor supplied by the test host.
// RU: Выполнить один раз с дескриптором владельца, выданным тестовым хостом.
function onEnterFrame() {
    if (!importProofDone) {
        var importedCounter = rpc.acceptIntRef(wireRef);
        var refs = channel.create("ref", 2);
        var forwarded = refs.send(ref importedCounter);
        var snapshot = importedCounter;
        var echoed = refs.recv();
        echoed = 111;
        _root.proofBefore = snapshot;
        _root.proofAfter = importedCounter;
        var closed = refs.close();
        importProofDone = true;
    }
}
