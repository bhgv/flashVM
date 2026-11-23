/* Copyright (c) 2026 bhgv
 * EN: Convert mapped channel elements to and from managed UniBus frames.
 * RU: Преобразование элементов Dis в управляемые кадры UniBus и обратно.
 */
#ifndef DVM_CHANNEL_VALUE_H
#define DVM_CHANNEL_VALUE_H
#include "managed_value.h"

struct Channel;
struct DVM_CallChannels;

/* EN: Build the frame schema of one channel type, or reject unsupported types.
 * RU: Собрать схему кадра из типа канала либо отклонить неподдерживаемый тип. */
int dvm_channel_value_shape(const char *type, char shape[STYX_SHAPE_MAX + 1]);

/* EN: Verify that the local channel's mover can transfer the advertised layout.
 * RU: Проверить, что обработчик локального канала переносит объявленную раскладку. */
int dvm_channel_value_mover(const struct Channel *channel, const char *shape);

/* EN: Initialize owned temporary slots for a receive, including nil pointers.
 * RU: Подготовить временные слоты для приёма, включая nil в полях ссылок. */
int dvm_channel_value_empty(const char *shape, DVM_StyxValueFrame *frame);

/* EN: Root already owned cells before publication, serialization or release.
 * RU: Укоренить уже удерживаемые поля до публикации, сериализации или освобождения. */
int dvm_channel_value_root(DVM_StyxValueFrame *frame);

/* EN: Serialize a privately owned frame to an exact portable value record.
 * RU: Сериализовать принадлежащий вызывающему кадр в точную переносимую запись. */
int dvm_channel_value_pack(const char *shape, const DVM_StyxValueFrame *frame,
    struct DVM_CallChannels *channels, void *bytes, size_t capacity, size_t *used);

/* EN: Construct an owned, rooted value frame from the borrowed wire record.
 * RU: Построить укоренённый кадр из заимствованной сетевой записи. */
int dvm_channel_value_unpack(const char *shape, const void *bytes, size_t size,
    struct DVM_CallChannels *channels, DVM_StyxValueFrame *frame);

#endif
