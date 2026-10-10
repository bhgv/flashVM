# Extending: prototype-backed native objects

The embedding example in `../../embedding/prototypes/` shows the underlying
object API. Extensions can create objects and set their prototype through the
same C ABI. Registration of native constructors and instance methods is still
planned; this folder is a documented design placeholder, not a working plugin.
