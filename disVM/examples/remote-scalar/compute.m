# Copyright (c) 2026 bhgv
# EN: Ordinary shared interface; no wire fields or native frame layout.
# RU: Обычный общий интерфейс, без wire fields и native frame layout.
Compute: module
{
    PATH: con "implementation.dis"; # Ordinary description, not transport authority.
    Scalar: type int; # Source alias resolved by the existing C semantic pass.
    compute: fn(value: Scalar): Scalar; # Copied scalar input/result / Scalar input/result.
};
