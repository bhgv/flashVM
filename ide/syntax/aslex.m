# Worker-private facade over generated Coco/R scanner tables.
AslexParser: module {
    PATH: con "aslex.dis"; # Native-cell module compiled with the matching runtime.
    # Borrow immutable bytes until reset; no UI or channel operations.
    reset: fn(data: array of byte);
    # Restart only at a caller-proven normal-state boundary; zero rejects byte bounds.
    resetat: fn(data: array of byte, first: int): int;
    # Return lexical class and half-open byte range; -1 marks EOF.
    next: fn(): (int, int, int);
};
