
# Blynk.Edgent for Particle

![image](https://github.com/blynkkk/blynkkk.github.io/raw/master/images/GithubBanner.jpg?raw=1)

**Blynk.Edgent** enables smart device management, provisioning, and connectivity for Particle boards using the Blynk IoT platform.

## Features

- `Blynk.Edgent` - Manages device states and connection logic.
- `Blynk.Inject` - Handles BLE-assisted device and network provisioning.
- `Blynk Library` - Implements the Blynk protocol and API.
- `NetMgr` - Unified `WiFi`, `Ethernet`, and `Cellular` network management
- `Preferences` - Configuration storage compatible with ESP32, ESP8266, and Particle platforms.

Supported Particle boards:
- `Muon`, `Argon`, `Boron`, `Photon 2`, `Tracker`, `P2`, `BSOM`, `MSOM`

Tested Particle OS versions:
- `5.8.0`

## Getting Started

### 0. Prerequisites

- Sign up/Log in to your [Blynk Account](https://blynk.cloud)
- Install **Blynk IoT App**:
  - [iOS](https://apps.apple.com/us/app/blynk-iot/id1559317868)
  - [Android](https://play.google.com/store/apps/details?id=cloud.blynk)
- Install [Particle CLI](https://docs.particle.io/getting-started/developer-tools/cli/)

### 1. Build and Upload your firmware

In `src/main.cpp`, set your [`BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME`](https://bit.ly/BlynkInject).

Compile and upload the firmware using `Particle CLI`:

```sh
particle flash MyPhoton2 --target=5.8.0
```

### 2. Connect your device

Use the **Blynk IoT App** to [provision your device](https://docs.blynk.io/en/blynk.edgent/overview#how-to-connect-a-device-with-blynk.edgent) and connect it to your Blynk account.

### Debugging

Adjust the logging level in `lib/NetMgr/NetMgrLogger.h`:

```h
#define LOGGER_PRINT      Serial
#define LOGGER_LOG_LEVEL  3
```

### Enabling SSL security

In `project.properties`, uncomment `dependencies.ArduinoBearSSL`.

In `lib/BlynkEdgent/src/EdgentSettings.h`, uncomment `#define CONFIG_USE_SSL`.

## Further Reading

- [Blynk.Edgent Overview](https://docs.blynk.io/en/blynk.edgent/overview)
- [Preferences library](https://github.com/vshymanskyy/Preferences)

