// Copyright (c) 2026 bhgv
// EN: Publish scalar and channel callbacks using only the stock player API.
// RU: Опубликовать скалярный и канальный колбэки через API штатного плеера.

// EN: Publish a typed queue whose first value comes from the same Math receiver.
// RU: Опубликовать очередь с первым значением от того же получателя Math.
function onEnterFrame() {
    if (!published) {
        Math.exportedCount = 7;
        scalarDescriptor = "normalize,abs";
        scalar = scalarDescriptor.split(",");
        factoryDescriptor = "queue,abs";
        factory = factoryDescriptor.split(",");
        factoryLength = factory.push(166697325, "(i)", "(c[i])", "channel", 1);
        propertyDescriptor = "count,exportedCount,var,rw";
        property = propertyDescriptor.split(",");
        published = vfs.publishObject("module-channel.dis", Math, scalar, factory, property);
    }
}
