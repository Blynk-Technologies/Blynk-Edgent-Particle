/*
 * Copyright (c) 2024 Blynk Technologies Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Fill in information from your Blynk Template here */
/* Read more: https://bit.ly/BlynkInject */
#define BLYNK_TEMPLATE_ID           "TMPxxxxxx"
#define BLYNK_TEMPLATE_NAME         "Device"

/* The firmware version (used for OTA updates) */
#define BLYNK_FIRMWARE_VERSION      "0.1.0"

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

#include <BlynkEdgent.h>

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_WARN, {
  { "app",               LOG_LEVEL_ALL   },
  { "system.ctrl.ble",   LOG_LEVEL_ALL   },
  { "wiring.ble",        LOG_LEVEL_ALL   },
  { "blynk.inject",      LOG_LEVEL_INFO  },
  { "prefs",             LOG_LEVEL_WARN  },
});

/*
 * Remote Terminal
 */

#include <utility/BlynkStreamMulti.h>
MultiStream    MultiSerial;
WidgetTerminal VirtualSerial;
BLYNK_ATTACH_WIDGET(VirtualSerial, V64);

/*
 * Main
 */

BlynkTimer timer;

void myTimer()
{
  // This function describes what will happen with each timer tick
  // e.g. writing sensor value to datastream V5
  Blynk.virtualWrite(V5, millis());  
}

void setup()
{
  Serial.begin(115200);
  waitFor(Serial.isConnected, 5000);
  Serial.println();

  // Set unlimited configuration mode retries (use only for testing!!!)
  BlynkEdgent.setConfigSkipLimit(0);

  // Setting interval to send data to Blynk Cloud to 1000ms. 
  // It means that data will be sent every ten seconds
  timer.setInterval(10000L, myTimer); 

  // Initialize Blynk.Edgent
  BlynkEdgent.begin();

  // Enable remote and local Edgent Console (optional)
  VirtualSerial.autoAppendLF();
  MultiSerial.addStream(BLYNK_PRINT);
  MultiSerial.addStream(VirtualSerial);
  BlynkEdgent.initConsole(MultiSerial);
}

void loop()
{
  delay(10);
  BlynkEdgent.run();
  timer.run(); 
}
