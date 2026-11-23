/* Copyright (c) 2026 bhgv
 * EN: Construct a bounded mux over the existing PHY.
 * RU: Создание ограниченного мультиплексора поверх PHY.
 */
/* EN/RU: See mux.h; ordinary construction retains the exact raw 9P framing and allocation bounds.
 * См. mux.h; ordinary constructor сохраняет raw 9P framing и allocation bounds.
 */


/* [standalone-contract:styxmuxcreatephy]
 * EN: styxmuxcreatephy: native client mux operations and lifetime.
 * RU: styxmuxcreatephy: нативные client-операции mux и время жизни.
 * phy (RemotePhy *phy): Physical-carrier capability; ownership transfer depends on the constructor/operation.
 * RU: Capability физического носителя; передача владения зависит от конструктора/операции.
 * capacity (unsigned capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * timeout (unsigned timeout): Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract.
 * RU: Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом.
 * limit (unsigned limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: StyxMux *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxMux *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxMux *styxmuxcreatephy(RemotePhy *phy, unsigned capacity, unsigned timeout, unsigned limit)
{
    return styxmuxcreatewalkphy(phy, capacity, timeout, limit, 0);
}
