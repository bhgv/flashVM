# Copyright (c) 2026 bhgv
# EN/RU: Per-launch native Unibus uses the generated ABI, not a renderer/VM bridge dependency.
# Per-launch Unibus использует generated ABI без зависимости от renderer.
target_sources(disvm PRIVATE host_interface/dispatch.c "${root}/disVM/host/unibus_binding.c")
