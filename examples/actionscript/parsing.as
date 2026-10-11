// API illustration; executable coverage: tests/global_parsing_avm1.cpp.
// Иллюстрация API; исполняемая проверка: tests/global_parsing_avm1.cpp.
trace(parseInt("0777"));       // 511, legacy AS2 octal / восьмеричное правило AS2
trace(parseInt("0777", 10));   // 777
trace(parseInt("3E8", 16));    // 1000
trace(parseFloat("12.5e1px")); // 125
trace(String(42));             // "42"
