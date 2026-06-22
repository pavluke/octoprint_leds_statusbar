/**
 * @file errors.h
 * @brief Validates secrets.h fields and exposes typed config constants.
 *
 * Must be included AFTER secrets.h — otherwise the macros being checked
 * aren't defined yet.
 *
 * If any field in secrets.h was left blank or at its default placeholder,
 * the build fails here with a clear compile error instead of unpredictable
 * runtime behavior (empty SSID, IP 0.0.0.0, port 0, etc.).
 *
 * The rest of the project (.ino) uses the constants defined at the bottom
 * of this file (WIFI_SSID, WIFI_PASSWORD, OCTOPRINT_IP, OCTOPRINT_HTTP_PORT,
 * OCTOPRINT_APIKEY) rather than the SECRET_* macros directly.
 */

#ifndef ERRORS_H
#define ERRORS_H

#include <Arduino.h>
#include <IPAddress.h>

#ifndef SECRET_WIFI_SSID
#error "errors.h: secrets.h is not included, or SECRET_WIFI_SSID is not defined."
#endif

#ifndef SECRET_WIFI_PASSWORD
#error "errors.h: secrets.h is not included, or SECRET_WIFI_PASSWORD is not defined."
#endif

#ifndef SECRET_OCTOPRINT_IP
#error "errors.h: secrets.h is not included, or SECRET_OCTOPRINT_IP is not defined."
#endif

#ifndef SECRET_OCTOPRINT_PORT
#error "errors.h: secrets.h is not included, or SECRET_OCTOPRINT_PORT is not defined."
#endif

#ifndef SECRET_OCTOPRINT_APIKEY
#error "errors.h: secrets.h is not included, or SECRET_OCTOPRINT_APIKEY is not defined."
#endif

// The preprocessor (#if) can't evaluate string contents — sizeof() and
// strcmp() are both compile-time, not preprocessor-time, constructs, so
// "#if (sizeof(STR) <= 1)" is invalid and fails to compile. We check for
// an empty string with a constexpr helper + static_assert instead, which
// runs at compile time, after the preprocessor has expanded the macros.
constexpr bool errorsIsEmptyString(const char* s) {
  return s[0] == '\0';
}

static_assert(!errorsIsEmptyString(SECRET_WIFI_SSID),
              "secrets.h: fill in SECRET_WIFI_SSID -- the name of your Wi-Fi network.");

static_assert(!errorsIsEmptyString(SECRET_WIFI_PASSWORD),
              "secrets.h: fill in SECRET_WIFI_PASSWORD -- your Wi-Fi network password.");

static_assert(!errorsIsEmptyString(SECRET_OCTOPRINT_APIKEY),
              "secrets.h: fill in SECRET_OCTOPRINT_APIKEY -- the Global API Key from OctoPrint settings.");

#if (SECRET_OCTOPRINT_PORT == 0)
#error "secrets.h: fill in SECRET_OCTOPRINT_PORT -- e.g. 5000."
#endif

// IPAddress takes 4 comma-separated numbers; #if can't compare the whole
// list at once, so we pull out the first byte via a helper macro and
// compare just that against 0 (a reserved range that doesn't show up on a
// real local network). Unlike the string checks above, this one stays a
// preprocessor #if because it only compares a single integer, which the
// preprocessor handles natively.
#define ERRORS_IP_FIRST_OCTET_HELPER(a, b, c, d) a
#define ERRORS_IP_FIRST_OCTET(ip) ERRORS_IP_FIRST_OCTET_HELPER(ip)

#if (ERRORS_IP_FIRST_OCTET(SECRET_OCTOPRINT_IP) == 0)
#error "secrets.h: fill in SECRET_OCTOPRINT_IP -- e.g. 192, 168, 1, 123."
#endif


/** Wi-Fi SSID to connect to. */
const char* const WIFI_SSID = SECRET_WIFI_SSID;

/** Wi-Fi password to connect with. */
const char* const WIFI_PASSWORD = SECRET_WIFI_PASSWORD;

/** IP address of the OctoPrint server on the local network. */
const IPAddress OCTOPRINT_IP(SECRET_OCTOPRINT_IP);

/** TCP port of the OctoPrint server. */
const int OCTOPRINT_HTTP_PORT = SECRET_OCTOPRINT_PORT;

/** Global API Key used to authorize requests to the OctoPrint REST API. */
const String OCTOPRINT_APIKEY = SECRET_OCTOPRINT_APIKEY;

#endif  // ERRORS_H