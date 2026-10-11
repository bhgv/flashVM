// API illustration; executable coverage: tests/standard_string_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/standard_string_avm1.cpp.
var text = new String("AБ😀Z");
trace(text.length);             // 5 UTF-16 units / кодовых единиц UTF-16
trace(text.substring(2, 4));     // 😀
trace(text.indexOf("Z"));        // 4
trace(text.charCodeAt(2));       // 55357 (high surrogate / старший суррогат)
trace(String.fromCharCode(65)); // A
// text.charAt(2) is currently unsupported: isolated surrogate cannot be stored.
// text.charAt(2) пока не поддержан: одиночный суррогат не представим в C ABI.
