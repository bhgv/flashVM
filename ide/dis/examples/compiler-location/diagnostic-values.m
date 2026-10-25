# Developer fixture: the integration test regenerates values from real diagnostics.
Source: con "/*中😀*/ trace(missing);\n"; # Exact UTF-8 source snapshot; the identifier is intentionally unresolved.
ByteFirst: con 18;                        # Inclusive byte offset of missing in that original source.
ByteLast: con 25;                         # Exclusive byte offset; convert the span before indexing Limbo characters.
