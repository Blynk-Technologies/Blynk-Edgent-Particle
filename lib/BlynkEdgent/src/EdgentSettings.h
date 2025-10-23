/*
 * Copyright (c) 2024 Blynk Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Advanced options
 */

#define CONFIG_COMMAND_NETMGR
#define CONFIG_COMMAND_SYS
//#define CONFIG_COMMAND_I2CDETECT
//#define CONFIG_COMMAND_FILESYS
//#define CONFIG_COMMAND_PREFS
//#define CONFIG_USE_SSL

#if defined(ESP32)
  #define BLYNK_MULTITHREADED
  #define CONFIG_USE_SSL
#elif defined(ESP8266)
  #define CONFIG_USE_SSL
#endif

#if !defined(BLYNK_DEVICE_PREFIX)
  #define BLYNK_DEVICE_PREFIX        "Blynk"
#endif
#if !defined(BLYNK_DEFAULT_SERVER)
  #define BLYNK_DEFAULT_SERVER       "blynk.cloud"
#endif

// Disable built-in analog and digital pin control
#define BLYNK_NO_BUILTIN
#define BLYNK_NO_DEFAULT_BANNER

// If max connection retries is exceeded, device enters ERROR state and reboots
#define WIFI_CLOUD_MAX_RETRIES        500
#define WIFI_NET_CONNECT_TIMEOUT      50000     // ms
#define WIFI_CLOUD_CONNECT_TIMEOUT    50000     // ms

