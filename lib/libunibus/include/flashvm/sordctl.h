/* Copyright (c) 2026 bhgv
 * EN: Named tag-zero ordered-lane negotiation, separate from numeric service-version registries.
 * RU: Именованное согласование линии с номерами через тег 0, отдельно от реестров числовых версий служб.
 */
#ifndef FLASHVM_SORDCTL_H
#define FLASHVM_SORDCTL_H
#include "sorder.h"
#define FVM_SORDER_SERVICE "unibus.order.v1" /* Exact nested identity; no global numeric service ID. */
#define FVM_SORDER_CTL_MAX 256U              /* Conservative full named-service payload capacity. */
#define FVM_SORDER_OFFER 1UL                 /* Offer one TX direction's wrapper tag and limits. */
#define FVM_SORDER_ACCEPT 2UL                /* Accept bounded limits for the exact proposal. */
#define FVM_SORDER_REJECT 3UL                /* Explicit rejection, echo the original proposal. */
#define FVM_SORDER_RENEW 4UL                 /* Final old ordinal for one inner tag in that lane. */
#define FVM_SORDER_ACK 5UL                   /* Exact renewal acknowledgement. */
#define FVM_SORDER_NACK 6UL                  /* Explicit renewal refusal. */
#ifdef __cplusplus
extern "C"
{
#endif


/* EN: Checked values, never native wire layout; offer fields differ from renewal fields as documented.
 * RU: Проверенные значения, не native-раскладка протокола; поля offer и renewal описаны отдельно.
 */
typedef struct FVM_SOrderControl
{
    unsigned long command;  /* One service-local command above. */
    FVM_SOrdinal operation; /* Nonzero correlation, local to proposal direction or renewal tag. */
    unsigned long lane;     /* Nonzero Uint32 outer wrapper tag, never an inner application grant. */
    unsigned long limit;    /* OFFER/ACCEPT/REJECT body limit; zero for renewal. */
    FVM_SOrdinal window;    /* OFFER/ACCEPT/REJECT future displacement; zero for renewal. */
    unsigned long tag;      /* Renewal's inner tag, including zero; zero in offers. */
    FVM_SOrdinal boundary;  /* Renewal's final old ordinal; zero in offers. */
} FVM_SOrderControl;

/* EN: Encode two complete nested S-records into disjoint output; no outer tag-zero wrapper.
 * Identity record tag0, command record tag1..6, six canonical unsigned integers in fixed order.
 * Borrow message/output; outputs unchanged on any failure, no allocation, C89 ABI.
 * RU: Кодировать две вложенные S-записи без внешнего tag0; шесть канонических чисел.
 * Буферы не пересекаются; отказ не меняет output/used, выделения памяти нет.
 */


/* [standalone-contract:fvm_sorder_control_write]
 * EN: fvm_sorder_control_write: named order-service offer/accept/renewal records.
 * RU: fvm_sorder_control_write: именованные order-service offer/accept/renewal записи.
 * message (const FVM_SOrderControl *message): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sorder_control_write(const FVM_SOrderControl *message,
                                             void *output,
                                             size_t capacity,
                                             size_t *used);


/* EN: Read exactly one named service body, copying scalars only on OK. UNKNOWN for other identities.
 * Known malformed messages return INVALID/checksum; trailing records/integers are invalid.
 * RU: Прочитать тело именованной службы; скаляры меняются лишь при OK, другая служба — UNKNOWN.
 * Ошибка известной службы и лишние данные отвергаются. Input заимствован до возврата.
 */


/* [standalone-contract:fvm_sorder_control_read]
 * EN: fvm_sorder_control_read: named order-service offer/accept/renewal records.
 * RU: fvm_sorder_control_read: именованные order-service offer/accept/renewal записи.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * message (FVM_SOrderControl *message): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sorder_control_read(const void *input, size_t size, FVM_SOrderControl *message);
#ifdef __cplusplus
}
#endif
#endif
