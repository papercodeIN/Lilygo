/**
 * @file debug_log.h
 * Crash-safe boot debugging: every line is flushed immediately so output
 * survives a panic or watchdog reset right after it, and every line carries
 * the internal-heap state (free / minimum-ever / largest block) so heap
 * exhaustion during BLE init is visible at the exact step it happens.
 */
#pragma once

namespace dbg {

// Human-readable name for the reason of the PREVIOUS reset
// (esp_reset_reason): POWERON, PANIC, TASK_WDT, BROWNOUT, ...
const char *reset_reason_name();

// Prints "[BOOT] <message> | int free N (min M) largest K" and flushes.
// Safe from any task context. `fmt` follows printf conventions.
void log(const char *fmt, ...);

} // namespace dbg
