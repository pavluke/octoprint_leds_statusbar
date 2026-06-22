/**
 * @file defines.h
 * @brief Printer statuses and conversion from OctoPrint API strings to enum.
 */

#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>

/**
 * @brief Printer/connection statuses rendered on the LED strip.
 *
 * These used to be string #defines — replaced with an enum: int comparison
 * instead of String::operator==, no risk of a typo in a literal, and the
 * compiler will flag an incomplete switch when a status is added.
 *
 * The order of values matters: it's used as an index into the STYLES[]
 * table in octoled.ino. When adding a new status, append it to the end of
 * both this enum and STYLES[] in sync.
 */
enum class PrinterStatus {
  PRINTING,             ///< Actively printing.
  PAUSED,               ///< Print job paused.
  COMPLETED,            ///< Print finished successfully (100%).
  IDLE,                 ///< Printer free, no print job running.
  OFFLINE_AFTER_ERROR,  ///< Printer went offline due to an error.
  OFFLINE,              ///< Printer unavailable (planned offline).
  NOT_CONNECTED         ///< OctoPrint API unreachable (network error, etc.).
};

/**
 * @brief Converts an OctoPrint API response into a PrinterStatus.
 *
 * OctoPrint returns the state as a string ("Printing", "Operational", etc.)
 * — all the "messy" parsing logic for that response is concentrated here,
 * in one place, instead of scattered across the code.
 *
 * @param apiState   Value of api.printJob.printerState from the OctoPrint API.
 * @param completion Current print progress (0-100); needed to distinguish
 *                    Idle from Completed — both arrive from the API as
 *                    "Operational".
 * @return The corresponding PrinterStatus. If apiState isn't recognized,
 *         returns NOT_CONNECTED as a safe fallback instead of undefined
 *         behavior.
 */
inline PrinterStatus parsePrinterStatus(const String& apiState, float completion) {
  if (apiState == "Printing")               return PrinterStatus::PRINTING;
  if (apiState == "Paused")                 return PrinterStatus::PAUSED;
  if (apiState == "Offline after error")    return PrinterStatus::OFFLINE_AFTER_ERROR;
  if (apiState == "Offline")                return PrinterStatus::OFFLINE;
  if (apiState == "Operational") {
    if (completion >= 100) return PrinterStatus::COMPLETED;
    return PrinterStatus::IDLE;
  }
  return PrinterStatus::NOT_CONNECTED;
}

/**
 * @brief Returns a human-readable name for a status, for logging.
 *
 * @param status Printer status.
 * @return A string literal with the status name (e.g. "Printing"). Returns
 *         "Unknown" for unrecognized enum values.
 */
inline const char* printerStatusToString(PrinterStatus status) {
  switch (status) {
    case PrinterStatus::PRINTING:             return "Printing";
    case PrinterStatus::PAUSED:               return "Paused";
    case PrinterStatus::COMPLETED:            return "Completed";
    case PrinterStatus::IDLE:                 return "Idle";
    case PrinterStatus::OFFLINE_AFTER_ERROR:  return "Offline after error";
    case PrinterStatus::OFFLINE:              return "Offline";
    case PrinterStatus::NOT_CONNECTED:        return "Not connected";
    default:                                  return "Unknown";
  }
}

#endif  // DEFINES_H
