# This source is compiled by our Coco/R Limbo numeric frontend, not the bootstrap.
implement PowerFacts;

# One scalar result is captured by the ordinary FIR/Dis backend.
PowerFacts: module
{
    value: fn(): int;
};


# Visit eight values with both signs. Forcing input bit2 and multiplying by8
# forces product bit5 even while the remaining input bits stay dynamic.
# Sum eight copies of32, then subtract214 to return42. Only integer wrap is used.
value(): int
{
    total := 0;
    for (index := 0; index < 8; index++) {
        input := index - 4;
        product := (input | 5) * 8;
        total += product & 32;
    }
    return total - 214;
}
