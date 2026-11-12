# Copyright (c) 2026 bhgv
# EN: Compose channel and ALT source adapters; no mutable runtime state.
# RU: Объединить адаптеры каналов и ALT; изменяемого состояния VM нет.
from prepare_channel_constructors import adapt, adapt_channel_operand_replay
from prepare_channel_matches import adapt_wait_completion
from prepare_alt_results import (
    adapt_alt_completion,
    adapt_alt_result_guards,
    adapt_alt_header_replay,
    adapt_alt_selection_capture,
)
from prepare_alt_waits import adapt_alt_reservations, adapt_alt_preflight
from prepare_channel_queues import adapt_channel_finalizer, adapt_queue_holds
from prepare_channel_buffer import adapt_buffer_reservation
