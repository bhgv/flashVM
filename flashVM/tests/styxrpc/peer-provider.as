// Copyright (c) 2026 bhgv
// EN: Publish one typed named callback from a running FlashVM movie.
// RU: Опубликовать именованный типизированный колбэк из фильма FlashVM.


// EN: Export Math.abs as an int32 callback with a named alias.
// RU: Экспортировать Math.abs как колбэк int32 под именем magnitude.
peerDescriptorText = "magnitude,abs";
peerDescriptor = peerDescriptorText.split(",");
peerPublished = styxrpc.publishObject("peer.dis", Math, peerDescriptor);
