# Copyright (c) 2026 bhgv
# EN: Deliberately incompatible result type for actual compiler signature checks.
# RU: Намеренно несовместимый result type для проверки actual compiler signatures.
Wrong: module
{
    value: fn(): big; # Different Limbo type even on a host whose int/big storage sizes agree.
};
