/* Copyright (c) 2026 bhgv
 * EN: Shared native test adapter, never a production packet/reliability protocol.
 * RU: Общий native test adapter, не production packet/reliability protocol.
 */
#ifndef PHY_TEST_LIMITED_H
#define PHY_TEST_LIMITED_H
#include "phy.h"


/* EN/RU: Consume lower carrier always; owned bounded wrapper/NULL, optional test terminal-prefix error.
 * Принять lower carrier всегда; свой bounded wrapper/NULL, optional test prefix error. */
RemotePhy *phyfixture(RemotePhy *child, int terminal);
#endif
