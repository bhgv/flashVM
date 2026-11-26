// Copyright (c) 2026 bhgv
// EN: Delay the readiness marker until the caller has seen it absent.
// RU: Отложить публикацию признака готовности, пока вызывающий не увидит его отсутствие.


// EN: Publish the release callback, then the marker on the owner frame after release.
// RU: Опубликовать колбэк разрешения, затем признак готовности на кадре владельца.
function onEnterFrame() {
    if (!refGatePublished) {
        var release = _root.releaseRefPeer;
        refGatePublished = vfs.newnode("/ref-release", "x", release);
    }
    if (refPublishReady) {
        if (!refReadyPublished) {
            var ready = _root.releaseRefPeer;
            refReadyPublished = vfs.newnode("/ref-ready", "x", ready);
        }
    }
}
