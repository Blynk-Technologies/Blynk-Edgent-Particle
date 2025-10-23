/*
 * Copyright (c) 2024 Blynk Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(BLYNK_TEMPLATE_ID) || !defined(BLYNK_TEMPLATE_NAME)
  #error "Please specify your BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME. Read more: https://bit.ly/BlynkInject"
#endif

#if defined(BLYNK_AUTH_TOKEN)
  #error "BLYNK_AUTH_TOKEN configured in runtime, please remove it from static configuration"
#endif

#include <Particle.h>
#include <EdgentSettings.h>
#include <NetMgr.h>
#ifdef CONFIG_USE_SSL
#  include <BlynkSimpleParticleSSL.h>
#else
#  include <BlynkSimpleParticle.h>
#endif
#include <BlynkInject.h>
#include <BlynkSysUtils.h>
#include <Blynk/BlynkConsole.h>
#include <ConfigStore.h>

class Edgent {

public:

  typedef void (*callback0_t)(void);

  enum State {
    MODE_IDLE,
    MODE_WAIT_CONFIG,
    MODE_CONNECTING_NET,
    MODE_CONNECTING_CLOUD,
    MODE_RUNNING,
    MODE_RESET_CONFIG,
    MODE_ERROR,

    MODE_MAX_VALUE
  };

  static String getStateName(State m) {
    if (m > MODE_MAX_VALUE) return "";
    static const char* stateStr[MODE_MAX_VALUE+1] = {
      "IDLE",
      "WAIT CONFIG",
      "CONNECTING NET",
      "CONNECTING CLOUD",
      "RUNNING",
      "RESET CONFIG",
      "ERROR",

      "INIT"
    };
    return stateStr[m];
  }

  void setConfigTimeout(int timeout) {
    _configTimeoutMs = BlynkMathClamp(timeout, 60, 3600) * 1000;
  }

  void setConfigSkipLimit(int count) {
    if (count == 0) {
      _configSkipLimit = 0;
    } else {
      _configSkipLimit = BlynkMathClamp(count, 5, 50);
    }
  }

  bool begin()
  {
    if (!String(BLYNK_TEMPLATE_ID).startsWith("TMPL")) {
      BLYNK_LOG1(F("Invalid Template configuration"));
      return false;
    }

    systemInit(BLYNK_DEVICE_PREFIX, BLYNK_TEMPLATE_NAME);

    NetMgr.begin();

    _store.begin();
    printBanner();
    initConsoleCommands();

    if (isConfigured()) {
      setState(MODE_CONNECTING_NET);
    } else if (_configSkipLimit &&
               (_store.getConfigSkipped() >= int(_configSkipLimit)))
    {
      setState(MODE_IDLE);
    } else {
      setState(MODE_WAIT_CONFIG);
    }

    return true;
  }

  void initConsole(Stream& stream);

  void run() {
    _timer.run();
    _console.run();

    NetMgr.run();

    switch (_state) {
    case MODE_IDLE:             stateIdle();              break;
    case MODE_WAIT_CONFIG:      stateConfig();            break;
    case MODE_CONNECTING_NET:   stateConnectingNet();     break;
    case MODE_CONNECTING_CLOUD: stateConnectingCloud();   break;
    case MODE_RUNNING:          stateRunning();           break;
    case MODE_RESET_CONFIG:     stateResetConfig();       break;
    default:                    stateError();             break;
    }
  }

private:

  void initConsoleCommands();

  /*
   * States
   */

  void stateConfig() {
    if (isEnteringState()) {
      _inject._config.host = BLYNK_DEFAULT_SERVER;

      _inject.setProvisionCallback(provisionCb);
      _inject.begin(systemGetDeviceName(),
                    BLYNK_DEVICE_PREFIX,
                    BLYNK_TEMPLATE_ID,
                    BLYNK_FIRMWARE_TYPE,
                    BLYNK_FIRMWARE_VERSION);

      setStateEntered();
    }
    _inject.run();
    if (millis() - _stateChangeTime > _configTimeoutMs) {
      if (_inject.isUserConfiguring()) {
        _stateChangeTime = millis(); // restart timer
        return;
      }
      // Write to NVM only if configSkipLimit is in use
      if (_configSkipLimit) {
        _store.storeConfigSkipped();
      }
      stopConfig();
    }
  }

  void stateIdle() {
    if (isEnteringState()) {
      if (_prevState == MODE_WAIT_CONFIG) {
        _inject.end();
      }
      // TODO: disable NetMgr?

      setStateEntered();
    }
  }

  void stateConnectingNet() {
    if (isEnteringState()) {
      if (_prevState == MODE_WAIT_CONFIG) {
        _inject.end();
      }
      NetMgr.allOn();
      setStateEntered();
    }

    if (NetMgr.isAnyConnected()) {
      _retriesNet = WIFI_CLOUD_MAX_RETRIES;
      setState(MODE_CONNECTING_CLOUD);
    } else if (millis() - _stateChangeTime > WIFI_NET_CONNECT_TIMEOUT) {
      BLYNK_LOG1(F("Network connection timeout"));
      if (--_retriesNet <= 0) {
        _inject.setLastError(BlynkInject::ERROR_NETWORK);

        // If setting not saved -> return to config mode
        if (!_store.isSaved()) {
          setState(MODE_WAIT_CONFIG);
        } else {
          setState(MODE_ERROR);
        }
      } else {
        setState(MODE_CONNECTING_NET, true);
      }
    }
  }

  void stateConnectingCloud() {
    if (isEnteringState()) {
      Particle.connect();

      Blynk.config(_store.getBlynkAuth().c_str(),
                   _store.getBlynkHost().c_str());
      Blynk.connect(0); // Start connecting, wait 0ms

      setStateEntered();
    }

    Blynk.run();

    if (Blynk.connected()) {
      if (!_store.isSaved()) {
        _inject.setLastError(BlynkInject::ERROR_NONE);
        _store.commit();

        BLYNK_LOG1(F("Config saved."));

        if (_onInitialConnection) { _onInitialConnection(); }
      }
      _retriesCloud = WIFI_CLOUD_MAX_RETRIES;
      systemStats.trackConnected();
      setState(MODE_RUNNING);

      if (_onStartupConnection) {
        String curr_fw = BLYNK_FIRMWARE_VERSION;
        String prev_fw = _store.getFirmwareVer();
        if (curr_fw != prev_fw) {
          if (prev_fw.length()) {
            Blynk.logEvent("sys_ota", String("Firmware updated from ") + prev_fw + " to " + curr_fw);
          }
          _store.storeFirmwareVer(curr_fw);
        }

        Blynk.sendInternal("meta", "set", "Device UID",   systemGetDeviceUID());
        Blynk.sendInternal("meta", "set", "Hotspot Name", systemGetDeviceName());

        if (_onStartupConnection != (callback0_t)1) {
          _onStartupConnection();
        }
        _onStartupConnection = NULL;
      }

    } else if ((_isTokenInvalid = Blynk.isTokenInvalid())) {
      if (!_store.isSaved()) {
        _inject.setLastError(BlynkInject::ERROR_TOKEN);
      }
      setState(MODE_WAIT_CONFIG); // TODO: retry after timeout
    } else if (!NetMgr.isAnyConnected()) {
      setState(MODE_CONNECTING_NET);
    } else if (millis() - _stateChangeTime > WIFI_NET_CONNECT_TIMEOUT) {
      BLYNK_LOG1(F("Cloud connection timeout"));
      if (--_retriesCloud <= 0) {
        _inject.setLastError(BlynkInject::ERROR_CLOUD);

        // If setting not saved -> return to config mode
        if (!_store.isSaved()) {
          setState(MODE_WAIT_CONFIG);
        } else {
          setState(MODE_ERROR);
        }
      } else {
        setState(MODE_CONNECTING_CLOUD, true);
      }
    }
  }

  void stateRunning() {
    if (!Blynk.connected()) {
      systemStats.trackDisconnected();
      if (NetMgr.isAnyConnected()) {
        systemStats.cloud_drops++;
        setState(MODE_CONNECTING_CLOUD);
      } else {
        systemStats.network_drops++;
        setState(MODE_CONNECTING_NET);
      }
    }
    Blynk.run();
  }

  void stateResetConfig() {
    BLYNK_LOG1(F("Resetting configuration!"));
    _store.erase();
    //NetMgr.clearAllNetworks();
    setState(MODE_WAIT_CONFIG);
  }

  void stateError() {
    if (millis() - _stateChangeTime > 10000) {
      BLYNK_LOG1(F("Restarting after error."));
      systemReboot();
    }
  }

public:

  void onStateChange(callback0_t f) {
    _onStateChange = f;
  }

  void onInitialConnection(callback0_t f) {
    _onInitialConnection = f;
  }

  void onStartupConnection(callback0_t f) {
    _onStartupConnection = f;
  }

  void onUserInitiatedReboot(callback0_t f) {
    _onUserInitiatedReboot = f;
  }

  void onConfigChange(callback0_t f) {
    _onConfigChange = f;
  }

  State getState() { return _state; }

  void setState(State m, bool reenter = false) {
    if (m >= MODE_MAX_VALUE) return;

    if (_state != m || reenter) {
      BLYNK_LOG3(getStateName(_state), " => ", getStateName(m));
      _prevState = (reenter) ? MODE_MAX_VALUE : _state;
      _state = m;
      _stateChangeTime = millis();

      if (_onStateChange) { _onStateChange(); }
    }
  }

  void startConfig() {
    Blynk.disconnect();
    setState(MODE_WAIT_CONFIG);
  }

  void stopConfig() {
    if (_store.isConfigured() && !_isTokenInvalid) {
      setState(MODE_CONNECTING_NET);
    } else {
      //NetMgr.allOff();
      setState(MODE_IDLE);
    }
  }

  void migrateAuthToken(String auth) {
    _store.setBlynkAuth(auth);
    _store.commit();
    setState(MODE_CONNECTING_NET);
  }

  bool isConfigured() {
    return _store.isConfigured() && NetMgr.isAnyConfigured();
  }

  void startInitialConnection() {
    _retriesNet = _retriesCloud = 1;
    setState(MODE_CONNECTING_NET);
  }

  void resetConfig() {
    if (getState() != MODE_WAIT_CONFIG) {
      setState(MODE_RESET_CONFIG);
    }
  }

  BlynkConsole&         getConsole()      { return _console; }
  BlynkInject::Config&  getInjectConfig() { return _inject._config; }

private:

  static void provisionCb();

  void provisioned() {
    if (_inject._config.intf == "wifi") {
#ifdef NetMgr_WiFi
      // TODO: static IP
      NetMgrWiFi.addNetwork(_inject._config.ssid, _inject._config.pass);
#endif
    }
    _store.setBlynkHost(_inject._config.host);
    _store.setBlynkAuth(_inject._config.auth);

    if (_onConfigChange) { _onConfigChange(); }

    startInitialConnection();
  }

  void printBanner()
  {
#ifdef BLYNK_PRINT
    Blynk.printBanner();
    BLYNK_LOG("----------------------------------------------------");
    BLYNK_LOG(" Device:    %s", systemGetDeviceName().c_str());
    BLYNK_LOG(" Version:   %s (build %s)", BLYNK_FIRMWARE_VERSION, __DATE__ " " __TIME__);
    BLYNK_LOG(" UID:       %s", systemGetDeviceUID().c_str());
    if (_store.isConfigured()) {
      BLYNK_LOG(" Token:     %s - •••• - •••• - ••••", _store.getBlynkAuth().substring(0,4).c_str());
    }
    BLYNK_LOG(" Platform:  %s", BLYNK_INFO_DEVICE);
    BLYNK_LOG("----------------------------------------------------");
#endif
  }

private:

  BlynkTimer    _timer;
  BlynkConsole  _console;
  BlynkInject   _inject;
  ConfigStore   _store;

  uint32_t      _stateChangeTime = 0;
  State         _state          = MODE_MAX_VALUE;
  State         _prevState      = MODE_MAX_VALUE;

  int           _retriesNet     = WIFI_CLOUD_MAX_RETRIES;
  int           _retriesCloud   = WIFI_CLOUD_MAX_RETRIES;
  unsigned      _configTimeoutMs = 5*60*1000;
  unsigned      _configSkipLimit = 10;
  bool          _isTokenInvalid = false;

  callback0_t   _onStateChange       = NULL;
  callback0_t   _onInitialConnection = NULL;
  callback0_t   _onStartupConnection = (callback0_t)1;
  callback0_t   _onUserInitiatedReboot = NULL;
  callback0_t   _onConfigChange = NULL;

  bool isEnteringState() { return _state != _prevState; }
  void setStateEntered() { _prevState = _state; }

} BlynkEdgent;

void Edgent::provisionCb() {
  BlynkEdgent.provisioned();
}

#include <BlynkEdgentConsole.h>

BLYNK_WRITE(InternalPinDBG) {
  BlynkEdgent.getConsole().runCommand(param.asStr());
}

