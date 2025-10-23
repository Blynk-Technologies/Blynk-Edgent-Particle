
# Blynk.Edgent for Particle

![image](https://github.com/blynkkk/blynkkk.github.io/raw/master/images/GithubBanner.jpg?raw=1)

## Features

- `Blynk.Edgent` - Device state management
- `Blynk.Inject` - BLE-assisted Device and Network Provisioning
- `Blynk library` - Blynk protocol and API implementation
- `NetMgr` - Unified `WiFi`, `Ethernet`, `Cellular` network management
- `Preferences` - Configuration storage for Particle and ESP8266 platforms, provides compatibility with the native ESP32 Preferences library.

Supported Particle boards:
- `Muon`, `Argon`, `Boron`, `Photon 2`, `Tracker`, `P2`, `BSOM`, `MSOM`

Tested Particle OS versions:
- `5.8.0`

## Getting Started

- Sign up/Log in to your [Blynk Account](https://blynk.cloud)
- Install **Blynk IoT App** for [iOS](https://apps.apple.com/us/app/blynk-iot/id1559317868) or [Android](https://play.google.com/store/apps/details?id=cloud.blynk)
- Install [**Particle CLI**](https://docs.particle.io/getting-started/developer-tools/cli/)

## Build and Upload your firmware

In `src/main.cpp`, set up your [`BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME`](https://bit.ly/BlynkInject).

Then, use `Particle CLI` to build the sample project:

```sh
particle flash MyPhoton2 --target=5.8.0
```

## Use your Blynk App to provision your device

## Debugging

You can change the logging level by editing `lib/NetMgr/NetMgrLogger.h`:

```h
#define LOGGER_PRINT      Serial
#define LOGGER_LOG_LEVEL  3
```
