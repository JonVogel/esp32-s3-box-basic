// See web_files.h for design notes.

#include "web_files.h"

// flash_trace.h lived in the sibling ti-extended-basic-esp32 project for
// correlating display tearing with NVS write events on the 8048S043C's
// RGB panel. The Box-3 has no such problem (SPI display, not RGB), so
// flash tracing isn't needed here — stub the three macros out so the
// call sites compile unchanged. If we ever need flash timing data on
// Box-3, restore flash_trace.h from the sibling project.
#define FLASH_TRACE_START(tag) ((void)0)
#define FLASH_TRACE_END(tag)   ((void)0)
#define FLASH_TRACE_MARK(tag)  ((void)0)

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>

namespace webfiles
{
  // State.
  static AsyncWebServer s_server(80);
  static bool s_serverRunning = false;
  static bool s_announcedConnected = false;   // edge-print "WiFi: IP=..."
  static bool s_busy = false;                  // RUN active?

  static const char* NVS_NAMESPACE = "wifi";

  // Read SSID + pass from NVS into the given buffers. Returns true if
  // a non-empty SSID was found. Pass buffers may be small; long
  // passphrases get truncated but that's an unrealistic configuration
  // (max is 63 chars per WPA2).
  static bool readCreds(char* ssid, int ssidSize,
                        char* pass, int passSize)
  {
    Preferences p;
    if (!p.begin(NVS_NAMESPACE, /*readonly=*/true))
    {
      return false;
    }
    size_t sLen = p.getString("ssid", ssid, ssidSize);
    size_t pLen = p.getString("pass", pass, passSize);
    p.end();
    (void)pLen;
    return sLen > 0 && ssid[0] != '\0';
  }

  static void startServerOnce()
  {
    if (s_serverRunning) return;

    // /api/status — small JSON used by the front-end to know whether
    // the board is currently running a BASIC program (so it can
    // disable uploads in the UI). Also useful as a "is the board
    // alive?" ping.
    s_server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
      char body[160];
      snprintf(body, sizeof(body),
               "{\"busy\":%s,\"ssid\":\"%s\",\"ip\":\"%s\",\"rssi\":%d}",
               s_busy ? "true" : "false",
               WiFi.SSID().c_str(),
               WiFi.localIP().toString().c_str(),
               (int)WiFi.RSSI());
      req->send(200, "application/json", body);
    });

    // Catch-all so a bare browser hit doesn't 404 confusingly while
    // we don't have a real UI yet. Real / handler lands in step 2.
    s_server.onNotFound([](AsyncWebServerRequest* req) {
      req->send(200, "text/plain",
                "TI Extended BASIC web file manager\n"
                "Endpoints so far: /api/status\n");
    });

    s_server.begin();
    s_serverRunning = true;
    Serial.println("webfiles: HTTP server listening on :80");
  }

  // Bring WiFi up in STA mode with the given creds. Non-blocking —
  // tick() will notice the connection and print the IP when it lands.
  //
  // Coexistence tuning for the Sunton 8048S043C: WiFi and the RGB
  // panel both contend for the PSRAM bus, and WiFi bursts can starve
  // the bounce-buffer refill ISR and visibly tear the display. The
  // settings below keep the radio as quiet as we can while remaining
  // online:
  //   - WIFI_PS_MAX_MODEM: longer sleep windows than MIN_MODEM (~200ms
  //     vs ~70ms). Trade response latency for fewer radio wakeups.
  //   - setTxPower(WIFI_POWER_8_5dBm): half the TX duration of the
  //     19.5 dBm default. Range cost is fine for LAN file transfer.
  static void connectWith(const char* ssid, const char* pass)
  {
    Serial.printf("webfiles: connecting to '%s'\n", ssid);
    FLASH_TRACE_START("wifi-begin");
    if (WiFi.getMode() == WIFI_OFF)
    {
      WiFi.mode(WIFI_STA);
      WiFi.setSleep(WIFI_PS_MAX_MODEM);
      WiFi.setTxPower(WIFI_POWER_8_5dBm);
    }
    else
    {
      WiFi.disconnect(false /*wifi_off*/, false /*erase config*/);
    }
    WiFi.begin(ssid, pass);
    s_announcedConnected = false;
    FLASH_TRACE_END("wifi-begin");
  }

  void begin()
  {
    char ssid[40] = {0};
    char pass[72] = {0};
    if (!readCreds(ssid, sizeof(ssid), pass, sizeof(pass)))
    {
      Serial.println("webfiles: no WiFi credentials in NVS. "
                     "Type CALL WIFI(\"ssid\",\"pass\") to set them.");
      return;
    }
    connectWith(ssid, pass);
  }

  void tick()
  {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck < 250) return;
    lastCheck = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
      if (!s_announcedConnected)
      {
        s_announcedConnected = true;
        FLASH_TRACE_MARK("wifi-associated");   // RF calibration NVS write fires here
        Serial.printf("webfiles: WiFi up. IP=%s  RSSI=%ddBm\n",
                      WiFi.localIP().toString().c_str(),
                      (int)WiFi.RSSI());
        startServerOnce();
      }
    }
    else
    {
      s_announcedConnected = false;
      // No explicit reconnect — the ESP32 WiFi driver retries on its
      // own with the credentials WiFi.begin() stashed. Just keep ticking.
    }
  }

  bool setCredentials(const char* ssid, const char* pass)
  {
    if (!ssid || ssid[0] == '\0') return false;
    FLASH_TRACE_START("nvs-wifi-creds");
    Preferences p;
    if (!p.begin(NVS_NAMESPACE, /*readonly=*/false))
    {
      FLASH_TRACE_END("nvs-wifi-creds");
      return false;
    }
    p.putString("ssid", ssid);
    p.putString("pass", pass ? pass : "");
    p.end();
    FLASH_TRACE_END("nvs-wifi-creds");
    connectWith(ssid, pass ? pass : "");
    return true;
  }

  void radioOff()
  {
    if (WiFi.getMode() == WIFI_OFF)
    {
      Serial.println("webfiles: radio already off");
      return;
    }
    if (s_serverRunning)
    {
      s_server.end();
      s_serverRunning = false;
    }
    WiFi.disconnect(true /*wifi_off*/, false /*keep config*/);
    WiFi.mode(WIFI_OFF);
    s_announcedConnected = false;
    Serial.println("webfiles: radio off");
  }

  void radioOn()
  {
    char ssid[40] = {0};
    char pass[72] = {0};
    if (!readCreds(ssid, sizeof(ssid), pass, sizeof(pass)))
    {
      Serial.println("webfiles: radio on requested but no creds in NVS");
      return;
    }
    connectWith(ssid, pass);
  }

  void forget()
  {
    FLASH_TRACE_START("nvs-wifi-clear");
    Preferences p;
    if (p.begin(NVS_NAMESPACE, /*readonly=*/false))
    {
      p.clear();
      p.end();
    }
    FLASH_TRACE_END("nvs-wifi-clear");
    if (s_serverRunning)
    {
      s_server.end();
      s_serverRunning = false;
    }
    WiFi.disconnect(true, true);
    s_announcedConnected = false;
    Serial.println("webfiles: WiFi credentials cleared.");
  }

  void status(char* out, int outSize)
  {
    if (!out || outSize <= 0) return;
    wl_status_t s = WiFi.status();
    if (s == WL_CONNECTED)
    {
      snprintf(out, outSize,
               "%s  IP=%s  RSSI=%ddBm  ONLINE",
               WiFi.SSID().c_str(),
               WiFi.localIP().toString().c_str(),
               (int)WiFi.RSSI());
    }
    else
    {
      // Distinguish "no creds at all" from "creds present, not connected"
      char ssid[40] = {0};
      char pass[72] = {0};
      bool haveCreds = readCreds(ssid, sizeof(ssid), pass, sizeof(pass));
      if (!haveCreds)
      {
        snprintf(out, outSize, "OFFLINE (no credentials set)");
      }
      else
      {
        snprintf(out, outSize, "%s  OFFLINE (status=%d)", ssid, (int)s);
      }
    }
  }

  void setBusy(bool busy)
  {
    s_busy = busy;
  }
}
