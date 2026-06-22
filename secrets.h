/**
 * @file secrets.h
 * @brief Personal connection details: Wi-Fi and OctoPrint access.
 *
 * IMPORTANT: this file contains sensitive data (Wi-Fi password, OctoPrint
 * API key) and must NOT be committed to version control. Make sure
 * `secrets.h` is listed in `.gitignore`.
 *
 * Fill in all the fields below with your own values. If anything is left
 * blank, errors.h (included from the .ino right after this file) will
 * stop the build with a clear compile error.
 */

#ifndef SECRETS_H
#define SECRETS_H

/** SSID of the home/work Wi-Fi network the device connects to. */
#define SECRET_WIFI_SSID ""

/** Password for the Wi-Fi network specified in WIFI_SSID. */
#define SECRET_WIFI_PASSWORD ""

/**
 * IP address of the OctoPrint server on the local network, 4 bytes
 * comma-separated.
 * Example: 192, 168, 1, 2
 * The first byte can't be 0 — errors.h uses that as the "field not filled
 * in" marker, so leave it as-is if you're unsure.
 */
#define SECRET_OCTOPRINT_IP 0, 0, 0, 0

/** Port OctoPrint accepts HTTP requests on (usually 5000 or 80). */
#define SECRET_OCTOPRINT_PORT 0

/**
 * Global API Key from the OctoPrint UI:
 * Settings -> API -> Global API Key.
 */
#define SECRET_OCTOPRINT_APIKEY ""

#endif  // SECRETS_H
