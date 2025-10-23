/*
 * Copyright (c) 2024 Blynk Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "BlynkInject.h"
#include "BlynkSysUtils.h"
#include <JsonWriter.h>

LOG_DEFINE_MODULE("blynk.inject")

BlynkInject::BlynkInject() {}

bool BlynkInject::isUserConfiguring() {
    return _user_started_configuring && _ble.isConnected();
}

void BlynkInject::begin(String name, String vendor, String tmpl_id, String fw_type, String fw_ver)
{
    if (_started) return;
    _started = true;

    _name = name.substring(0, 29);
    _vendor = vendor;
    _tmpl_id = tmpl_id;
    _fw_type = fw_type;
    _fw_ver = fw_ver;
    _user_started_configuring = false;

    _config.intf = _config.ssid = _config.pass = _config.auth = "";

#ifdef NetMgr_WiFi
    NetMgrWiFi.startConfig();
#endif
#ifdef NetMgr_Ethernet
    NetMgrEthernet.startConfig();
#endif
#ifdef NetMgr_Cellular
    NetMgrCellular.startConfig();
#endif

    _ble.begin(_name.c_str());
    LOG_I_MOD("BLE-assisted provisioning started");
}

void BlynkInject::end()
{
    _ble.end();
    _started = false;
    LOG_I_MOD("Provisioning finished");
}


void BlynkInject::run() {
    if (!_started) return;

    parse_message();
}

void BlynkInject::parse_message() {
    if (!_ble.available()) return;

    String cmd = _ble.read();
    JSONValue outerObj = JSONValue::parse((char*)cmd.c_str(), cmd.length());
    if (outerObj.type() != JSON_TYPE_OBJECT) {
      sendMsg(R"json({"t":"error","msg":"wrong format"})json");
      return;
    }

    // Process our received message. Get type first.
    JSONString t;
    {
        JSONObjectIterator iter(outerObj);
        while (iter.next()) {
            if (iter.name() == "t") {
                t = iter.value().toString();
            }
        }
    }

    if (t == "set") {
        bool foundInvalid = false;
        JSONObjectIterator item(outerObj);
        while (item.next()) {
          const JSONString& key = item.name();
          if      (key == "t")      { /* skip */ }
          else if (key == "if")     { _config.intf  = item.value().toString().data(); }
          else if (key == "ssid")   { _config.ssid  = item.value().toString().data(); }
          else if (key == "pass")   { _config.pass  = item.value().toString().data(); }
          else if (key == "blynk")  { _config.auth  = item.value().toString().data(); }
          else if (key == "host")   { _config.host  = item.value().toString().data(); }
          else if (key == "port")   { /* ignored */ }
          else if (key == "ip")     { _config.ip    = item.value().toString().data(); }
          else if (key == "mask")   { _config.mask  = item.value().toString().data(); }
          else if (key == "gw")     { _config.gw    = item.value().toString().data(); }
          else if (key == "dns")    { _config.dns   = item.value().toString().data(); }
          else if (key == "dns2")   { _config.dns2  = item.value().toString().data(); }
          else if (key == "save")   { _config.forceSave = true; }
          else                      { foundInvalid = true; }
        }
        if (!foundInvalid) {
          sendMsg(R"json({"t":"set_ok"})json");
        } else {
          sendMsg(R"json({"t":"set_fail"})json");
        }
    } else if (t == "connect") {
        if (_config.auth.length() == 32 &&
            ((_config.intf == "wifi" && _config.ssid.length()) ||
             (_config.intf == "cell") ||
             (_config.intf == "eth" ))
        ) {
            sendMsg(R"json({"t":"connecting"})json");

            if (provisionCb != nullptr) {
                provisionCb();
            }
        } else {
            LOG_W_MOD("Configuration invalid");
            sendMsg(R"json({"t":"connect_fail","msg":"configuration invalid"})json");
        }
    } else if (t == "info") {
        LOG_I_MOD("Sending board info");

        // Configuring starts with board info request
        _user_started_configuring = true;

        char buff[256];
        JsonBufferWriter writer(buff, sizeof(buff));
        writer.beginObject();
          writer["t"       ] = "info";
          writer["vendor"  ] = _vendor;
          writer["tmpl_id" ] = _tmpl_id;
          writer["fw_type" ] = _fw_type;
          writer["fw_ver"  ] = _fw_ver;
          writer["name"    ] = _name;
          writer["last_error"] = (int)_last_error;
        writer.endObject();
        sendMsg(writer.buffer(), writer.dataSize());
    } else if (t == "ifs") {
        LOG_I_MOD("Sending interface info");

        sendMsg(R"json({"t":"ifs_start"})json");
        delay(10);
        char buff[256];
#ifdef NetMgr_WiFi
        if (NetMgrWiFi.isHardwareAvailable()) {
          JsonBufferWriter writer(buff, sizeof(buff));
          writer.beginObject();
            writer["t"     ] = "if";
            writer["name"  ] = "wifi";
            writer["mac"   ] = NetMgrWiFi.getMacAddress();
            writer["scan"  ] = NetMgrWiFi.supportsScan()?1:0;
            writer["5ghz"  ] = NetMgrWiFi.supports5GHz()?1:0;
            writer["static_ip"] = NetMgrWiFi.supportsStaticIP()?1:0;
          writer.endObject();
          sendMsg(writer.buffer(), writer.dataSize());
          delay(10);
        }
#endif
#ifdef NetMgr_Cellular
        if (NetMgrCellular.isHardwareAvailable()) {
          JsonBufferWriter writer(buff, sizeof(buff));
          writer.beginObject();
            writer["t"     ] = "if";
            writer["name"  ] = "cell";
            writer["imei"  ] = NetMgrCellular.getIMEI();
            writer["imsi"  ] = NetMgrCellular.getIMSI();
            writer["iccid" ] = NetMgrCellular.getICCID();
            writer["scan"  ] = NetMgrCellular.supportsScan()?1:0;
            writer["pin"   ] = NetMgrCellular.supportsSimPin()?1:0;
            writer["apn"   ] = NetMgrCellular.supportsAPN()?1:0;
          writer.endObject();
          sendMsg(writer.buffer(), writer.dataSize());
          delay(10);
        }
#endif
#ifdef NetMgr_Ethernet
        if (NetMgrEthernet.isHardwareAvailable()) {
          JsonBufferWriter writer(buff, sizeof(buff));
          writer.beginObject();
            writer["t"     ] = "if";
            writer["name"  ] = "eth";
            writer["mac"   ] = NetMgrEthernet.getMacAddress();
            writer["status"] = NetMgrEthernet.getStatus();
            if (NetMgrEthernet.isConnected()) {
              writer["ip"  ] = NetMgrEthernet.getLocalIP();
            }
            writer["static_ip"] = NetMgrEthernet.supportsStaticIP()?1:0;
          writer.endObject();
          sendMsg(writer.buffer(), writer.dataSize());
          delay(10);
        }
#endif
        sendMsg(R"json({"t":"ifs_end"})json");
    } else if (t == "scan") {
#ifdef NetMgr_WiFi
        LOG_I_MOD("Scanning WiFi");
        sendMsg(R"json({"t":"scan_start"})json");

        int wifi_nets = NetMgrWiFi.scanNetworks();
        LOG_I_MOD("Found networks: %d", wifi_nets);
        wifi_nets = min(15, wifi_nets); // Use top 15 networks

        char buff[256];
        for (int i = 0; i < wifi_nets; i++) {
          String ssid, sec, bssid;
          int chan = -1, rssi = 0;
          NetMgrWiFi.scanGetResult(i, ssid, sec, rssi, bssid, chan);
          // skip weak and hidden networks
          if (rssi >= -90 && ssid.length()) {

            JsonBufferWriter writer(buff, sizeof(buff));
            writer.beginObject();
              writer["t"     ] = "scan";
              writer["ssid"  ] = ssid;
              writer["bssid" ] = bssid;
              writer["rssi"  ] = rssi;
              writer["sec"   ] = sec;
              writer["ch"    ] = chan;
            writer.endObject();
            sendMsg(writer.buffer(), writer.dataSize());
            delay(10);
          }
        }

        sendMsg(R"json({"t":"scan_end"})json");
        NetMgrWiFi.scanDelete();
#else
        sendMsg(R"json({"t":"error","msg":"no wifi"})json");
#endif
    } else if (t == "reset") {
#ifdef NetMgr_WiFi
        NetMgrWiFi.clearNetworks();
#endif
        sendMsg(R"json({"t":"reset_ok"})json");
    } else if (t == "reboot") {
        systemReboot();
    } else {
        sendMsg(R"json({"t":"error","msg":"invalid command"})json");
    }
}

void BlynkInject::setProvisionCallback(provisionCb_t* cb) {
    provisionCb = cb;
}
