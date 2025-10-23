
# Blynk.Edgent for Particle

Supported Particle boards:
- `Argon`, `Boron`, `Photon 2`, `P2`, `Tracker`

Tested Particle OS versions:
- `5.8.0`

![image](https://github.com/blynkkk/blynkkk.github.io/raw/master/images/GithubBanner.jpg?raw=1)

## Features

- `Blynk.Edgent` - Device state management
- `Blynk.Inject` - BLE-assisted Device and Network Provisioning
- `Blynk library` - Blynk protocol and API implementation
- `NetMgr` - Unified `WiFi`, `Ethernet`, `Cellular` network management
- `Preferences` - Configuration storage for Particle and ESP8266 platforms, provides compatibility with the native ESP32 Preferences library.

## Getting Started

- Sign up/Log in to your [Blynk Account](https://blynk.cloud)
- Install **Blynk IoT App** for [iOS](https://apps.apple.com/us/app/blynk-iot/id1559317868) or [Android](https://play.google.com/store/apps/details?id=cloud.blynk)
- Install [**Particle CLI**](https://docs.particle.io/getting-started/developer-tools/cli/)

## 1. Build and Upload your firmware

In `src/main.cpp`, set up your [`BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME`](https://bit.ly/BlynkInject).

Then, use `Particle CLI` to build the sample project:

```sh
particle flash MyPhoton2 --target=5.8.0
```

## Debugging

You can change the logging level by editing `lib/NetMgr/NetMgrLogger.h`:

```h
#define LOGGER_PRINT      Serial
#define LOGGER_LOG_LEVEL  3
```
