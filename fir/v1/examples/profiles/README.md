# FIR profiles and numeric literals

[main.fir](main.fir) traces 42 and 32. The header records AS2 provenance and the
restricted numeric frontend profile. It does not enable complete AS2 or target
capabilities: typed opcodes define semantics. Checks cover paired profiles,
signed hexadecimal literals and f64.div. Divide objects need SWL >=0.3.2.
The format stays FIR 1.0 and compiler output stays .swo.
