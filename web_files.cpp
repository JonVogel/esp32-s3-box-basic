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
#include <LittleFS.h>
#include <SD_MMC.h>
#include "ESP32_File_Handling/file_io.h"

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

    // /api/files?dev=FLASH | SDCARD | DSK1..DSK9 / DSKA..DSKZ
    // Returns JSON: {"device":"FLASH","files":[{"name":"X","size":N},...]}
    // or {"device":"...","error":"..."} on a problem. The front-end
    // populates a table per device; this is the only file-listing path.
    s_server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest* req) {
      String dev = req->hasParam("dev") ? req->getParam("dev")->value() : "FLASH";
      // Use a chunked-response so we don't have to size a buffer ahead.
      // The JSON we emit is small even on a full SD card (a few KB) so
      // a String-based response would also work, but the chunked form
      // avoids allocating the whole payload at once.
      AsyncWebServerResponse* resp = nullptr;

      // Enumerate into a String we then ship. Bounded by FIFO size of
      // the underlying device; in practice <2 KB even for SD with
      // hundreds of files. If this proves too memory-hungry we can
      // refactor to true streaming later.
      String body;
      body.reserve(2048);
      body += "{\"device\":\"";
      body += dev;
      body += "\",\"files\":[";
      bool first = true;
      auto appendFile = [&](const char* name, long size) {
        if (!first) body += ',';
        first = false;
        body += "{\"name\":\"";
        // Bare minimum escaping for JSON. File systems shouldn't normally
        // produce names with quotes or backslashes but be defensive.
        for (const char* p = name; *p; ++p)
        {
          if (*p == '"' || *p == '\\') body += '\\';
          body += *p;
        }
        body += "\",\"size\":";
        body += String(size);
        body += '}';
      };

      bool ok = true;
      String error;

      if (dev.equalsIgnoreCase("FLASH"))
      {
        File root = LittleFS.open("/");
        File f = root.openNextFile();
        while (f)
        {
          const char* n = f.name();
          bool hide = f.isDirectory() || n[0] == '.';
          if (!hide) appendFile(n, (long)f.size());
          f = root.openNextFile();
        }
      }
      else if (dev.equalsIgnoreCase("SDCARD") || dev.equalsIgnoreCase("SD"))
      {
        if (!fio::g_sdOk) { ok = false; error = "SD not present"; }
        else
        {
          File root = SD_MMC.open("/");
          File f = root.openNextFile();
          while (f)
          {
            const char* n = f.name();
            bool hide = f.isDirectory() || n[0] == '.' ||
                        strcasecmp(n, "System Volume Information") == 0;
            if (!hide) appendFile(n, (long)f.size());
            f = root.openNextFile();
          }
        }
      }
      else if (dev.length() >= 4 &&
               (dev[0] == 'd' || dev[0] == 'D') &&
               (dev[1] == 's' || dev[1] == 'S') &&
               (dev[2] == 'k' || dev[2] == 'K'))
      {
        int drive = fio::driveFromChar(dev[3]);
        dsk::DskImage* img = (drive > 0) ? fio::dskImage(drive) : nullptr;
        if (!img) { ok = false; error = "not mounted"; }
        else
        {
          dsk::DskImage::CatEntry ents[64];
          int n = img->listCatalog(ents, 64);
          for (int i = 0; i < n; i++)
          {
            // DSK catalog stores filenames as 10-char fixed; trim trailing
            // spaces for nicer JSON. Size is sector count (256 B each).
            char nm[12];
            strncpy(nm, ents[i].name, 10);
            nm[10] = '\0';
            for (int j = 9; j >= 0 && nm[j] == ' '; j--) nm[j] = '\0';
            appendFile(nm, (long)ents[i].totalSectors * 256L);
          }
        }
      }
      else
      {
        ok = false;
        error = "unknown device";
      }

      if (!ok)
      {
        body = "{\"device\":\"";
        body += dev;
        body += "\",\"error\":\"";
        body += error;
        body += "\"}";
      }
      else
      {
        body += "]}";
      }
      req->send(200, "application/json", body);
    });

    // Root HTML page. Plain, server-rendered shell that uses fetch() to
    // populate the file table from /api/files. No JS framework, no
    // external CDN — the whole UI is self-contained and works without
    // internet access from the browser (only LAN access to the device).
    s_server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
      // Listing all common devices up front so the dropdown is populated
      // statically. The user changes the device and the JS refetches.
      // Kept short and inline so the entire UI lives in this one file.
      static const char PAGE[] PROGMEM =
        "<!doctype html><html><head><meta charset=utf-8>"
        "<title>TI Extended BASIC Files</title>"
        "<style>"
        "body{font-family:monospace;background:#001;color:#cdf;margin:1em;}"
        "h1{color:#fff;border-bottom:1px solid #468;padding-bottom:.3em;}"
        "select,button{font-family:monospace;background:#024;color:#cdf;"
        "border:1px solid #468;padding:.3em .6em;}"
        "table{margin-top:1em;border-collapse:collapse;}"
        "th,td{padding:.2em .8em;text-align:left;border-bottom:1px solid #246;}"
        "th{color:#fff;}"
        "tr:hover{background:#012;}"
        ".err{color:#f88;}"
        ".muted{color:#789;font-size:.9em;}"
        "</style></head><body>"
        "<h1>TI Extended BASIC &mdash; File Browser</h1>"
        "<label>Device: <select id=dev>"
        "<option>FLASH</option><option>SDCARD</option>"
        "<option>DSK1</option><option>DSK2</option><option>DSK3</option>"
        "</select></label> "
        "<button onclick=refresh()>Refresh</button>"
        "<span class=muted id=status></span>"
        "<table id=ftbl><thead><tr><th>Name</th><th>Size</th></tr>"
        "</thead><tbody></tbody></table>"
        "<script>"
        "async function refresh(){"
        "const d=document.getElementById('dev').value;"
        "const s=document.getElementById('status');"
        "const tb=document.querySelector('#ftbl tbody');"
        "tb.innerHTML='';s.textContent='loading...';"
        "try{"
        "const r=await fetch('/api/files?dev='+encodeURIComponent(d));"
        "const j=await r.json();"
        "if(j.error){s.innerHTML='<span class=err>'+j.error+'</span>';return;}"
        "s.textContent=j.files.length+' file(s)';"
        "for(const f of j.files){"
        "const tr=document.createElement('tr');"
        "tr.innerHTML='<td>'+f.name+'</td><td>'+f.size+'</td>';"
        "tb.appendChild(tr);"
        "}}catch(e){s.innerHTML='<span class=err>'+e+'</span>';}}"
        "document.getElementById('dev').onchange=refresh;"
        "refresh();"
        "</script></body></html>";
      req->send_P(200, "text/html", PAGE);
    });

    // 404 for anything else. Endpoints handled above: GET /, /api/status,
    // /api/files. Future endpoints (upload, download, delete) hang off
    // /api/files/... so they shadow this.
    s_server.onNotFound([](AsyncWebServerRequest* req) {
      req->send(404, "text/plain", "not found\n");
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
