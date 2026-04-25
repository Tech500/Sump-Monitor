# Sump-Monitor

**Sump pit monitor using an ultrasonic transducer with text, email, and Alexa alerts.**

An ESP32-S3 based IoT project that continuously monitors the water level in a basement sump pit using a waterproof ultrasonic transducer. When the water level rises to a configured threshold, the system sends SMS text, email, and Alexa voice alerts — and logs all pit activity for historical review. A full Grafana dashboard, served directly from the ESP32-S3 web interface, provides real-time and historical visualization of pit conditions.

> ⚠️ **IMPORTANT — Only one board is supported.** See [Hardware Requirements](#hardware-requirements) before purchasing any components.

---

## Features

* **Ultrasonic water level sensing** — Waterproof JSN-SR04T transducer measures distance to the water surface continuously
* **SMS text alerts** — Sends text message notifications when water level exceeds configured thresholds
* **Email alerts** — Simultaneous email notifications for redundant coverage; a dedicated Google secondary account is strongly recommended
* **Alexa voice alerts** — When FLOODING is detected, Alexa announces "Sump pit flooding take immediate action" and plays three Claxion alarm blasts across all Echo devices via SinricPro
* **Grafana dashboard** — Live and historical pit condition graphs embedded via iframes in the ESP32-S3 web interface, auto-refreshing every 30 seconds
* **Multi-page web interface** — Full AsyncWebServer application with Main Menu, Graphs, File Browser, and File Reader routes
* **LittleFS logging** — Distance, condition, and pump runtime logs stored in ESP32-S3 flash; auto-purged every Sunday at midnight
* **InfluxDB time-series storage** — All pit events and readings stored on Raspberry Pi for long-term trending
* **Built-in file browser** — `/SdBrowse` route lists all LittleFS log files as clickable links; clicking any file opens the embedded File Reader
* **WiFiManager** — Captive portal for WiFi configuration; no hardcoded credentials required
* **ElegantOTA** — Over-the-air firmware updates via web browser
* **FTP access** — FTPServer provides direct access to LittleFS files
* **Arduino IDE compatible** — Written in C/C++ with ESP32 board support

---

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│              ESP32-S3 N16R8                         │
│                                                     │
│  JSN-SR04T ──► Ultrasonic sensing                   │
│                                                     │
│  AsyncWebServer routes:                             │
│    /sump      → Main Menu (live readings)           │
│    /graph     → Grafana iframe dashboard            │
│    /SdBrowse  → LittleFS log file browser           │
│    /show      → File Reader (index4.h)              │
│    404 handler→ URL → filename → redirect /show     │
│                                                     │
│  LittleFS logs:                                     │
│    Distance log  (DistanceToTarget + timestamp)     │
│    Condition log (event + distance + timestamp)     │
│    PUMP log      (pump runtime)                     │
│    Auto-purged every Sunday at midnight             │
│                                                     │
│  Alerts:                                            │
│    SMS  ──────────────────────────────► Phone       │
│    Email ─────────────────────────────► Inbox       │
│    SinricPro ─────────────────────────► Alexa       │
│    InfluxDB write ────────────────────► Raspberry Pi│
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│              Raspberry Pi (Required)                │
│                                                     │
│  InfluxDB  → time-series event & reading storage    │
│  Grafana   → dashboard panels (iframed in ESP32)    │
│  My Media  → serves Claxion3.mp3 to Alexa           │
│  Tailscale Funnel → secure remote access (port 80)  │
└─────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────┐
│              Alexa Echo Devices                     │
│                                                     │
│  SinricPro virtual sensors:                         │
│    FLOODING  → announces alert + 3 Claxion blasts   │
│    HIGHWATER → high water warning                   │
│    ALLCLEAR  → water level normal                   │
└─────────────────────────────────────────────────────┘
```

---

## Hardware Requirements

> ⚠️ **Critical — Only One Board Is Supported:**
> This project requires the **ESP32-S3 with N16R8** (16MB Flash / 8MB PSRAM).
> The combined memory demands of AsyncWebServer, LittleFS, ESP Mail Client, SinricPro,
> InfluxDB client, and WiFi libraries require the full N16R8 memory configuration.
> **No other ESP32 board is supported or will work.**
> Earlier versions of this project ran on an ESP32 DevKit V1 — the current codebase
> will not run on that hardware.

| Component | Description |
| --- | --- |
| **ESP32-S3 N16R8** | 16MB Flash / 8MB PSRAM — **required, no substitutions** |
| **JSN-SR04T** | Waterproof ultrasonic transducer — 5V, JST connector |
| **Voltage Divider** | Required on ECHO pin — ESP32-S3 is 3.3V only (see Wiring) |
| **Raspberry Pi** | Required — runs InfluxDB, Grafana, and My Media server |
| **Power Supply** | 5V regulated for ESP32-S3; separate 5V for JSN-SR04T |
| **Enclosure** | Weatherproof enclosure suitable for basement/pit environment |

---

## Software Requirements

* [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x)
* ESP32 board support package — Add the following URL to **File > Preferences > Additional Board Manager URLs**:

  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
* Required libraries (install via Arduino Library Manager):

```cpp
#include "Arduino.h"
#include <EMailSender.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebSocketsClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ElegantOTA.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <FS.h>
#include <LittleFS.h>
#include <FTPServer.h>
#include <HTTPClient.h>
#include <sys/time.h>
#include <time.h>
#include <Ticker.h>
#include <Wire.h>
#include <SinricPro.h>
#include "SinricProContactsensor.h"
#include <SinricProWebsocket.h>
#include "variableInput.h"
```

> ⚠️ **Important — Sketch Folder:**
> Copy the **entire Sump-Monitor sketch folder** to your Arduino sketches directory.
> The following files **must all be present** in the sketch folder together:
> - `sump-monitor_v9.5.ino`
> - `index1.h`
> - `index2.h`
> - `index3.h`
> - `index4.h`
> - `variableInput.h`

**Raspberry Pi services required:**
  + [InfluxDB](https://docs.influxdata.com/influxdb/v2/) — time-series database
  + [Grafana](https://grafana.com/docs/grafana/latest/) — dashboard and visualization
  + [My Media for Alexa](https://docs.bizmodeller.com/my-media-for-alexa/) — local media server for Alexa alarm sound
  + [Tailscale](https://tailscale.com) — secure remote access via Funnel

---

## Grafana Dashboard

The ESP32-S3 web interface serves an HTML page with embedded Grafana iframes, presenting a complete pit conditions dashboard auto-refreshing every 30 seconds. The dashboard includes:

* **All Events** — combined timeline of ALLCLEAR, FLOODING, and HIGHWATER events
* **High Water Events** — bar chart of high water occurrences
* **Flooding Events** — bar chart of flooding occurrences
* **All Clear Events** — bar chart of all-clear restorations
* **Pit Activity — Distance to Target** — continuous distance-to-water-surface readings in inches

Navigate to `http://<ESP32_IP>/graph` to view the dashboard.

---

## Web Interface Routes

| Route | Description |
| --- | --- |
| `/sump` | Main Menu — live water level readings and system status |
| `/graph` | Grafana iframe dashboard — pit conditions and event history |
| `/SdBrowse` | LittleFS file browser — lists all log files as clickable links |
| `/show` | File Reader — displays log file contents (via `index4.h` embedded script) |
| `404 handler` | Converts `.txt` URL to LittleFS filename (fn) and redirects to `/show` |

---

## LittleFS Logging

All logs are stored as `.txt` files in the ESP32-S3 LittleFS filesystem (enabled by the 16MB flash of the N16R8). Logs are automatically purged every **Sunday at midnight**.

| Log | Contents |
| --- | --- |
| **Distance Log** | DistanceToTarget readings with timestamp |
| **Condition Log** | Event name (FLOODING/HIGHWATER/ALLCLEAR), distance, timestamp |
| **PUMP Log** | Pump runtime duration with timestamp |

The `.txt` extension is used by the not-found handler to convert a clicked URL into a LittleFS filename, which is then internally redirected to the `/show` route where `index4.h` handles file reading and display.

---

## Alexa Integration

This project uses [SinricPro](https://sinric.pro) to create three virtual contact sensors that trigger Alexa routines:

| Sensor | Trigger | Alexa Action |
| --- | --- | --- |
| `FLOODING` | Water at flood level | Announces alert + plays 3 Claxion alarm blasts via My Media |
| `HIGHWATER` | Water at high-water mark | High water warning announcement |
| `ALLCLEAR` | Water returns to normal | All clear announcement |

> 💡 **SinricPro** offers a free plan covering 3 devices — exactly what this project needs.
> Additional devices are available at $3.00 per device per year.

For complete Alexa and SinricPro setup instructions see:
**[Alexa & SinricPro Setup Guide](https://github.com/Tech500/Sump-Monitor)** *(PDF in /docs)*

For My Media Raspberry Pi installation see:
**[My Media for Alexa Documentation](https://docs.bizmodeller.com/my-media-for-alexa/)**

---

## WiFi Configuration — WiFiManager

This project uses **WiFiManager** for WiFi setup — no hardcoded SSID or password required.

**First Boot / Network Not Found:**
1. ESP32-S3 starts automatically in Access Point mode
2. On your phone or laptop connect to WiFi AP: **`SumpMonitor-Setup`**
3. Browser will automatically open the WiFiManager portal — or manually browse to **`192.168.4.1`**
4. Enter your WiFi **SSID** and **Password** and click Save
5. Credentials are saved to flash — ESP32-S3 reboots and connects to your network

**Subsequent Boots:**
- If saved network is found → connects automatically ✅
- If saved network is **not found** → captive portal opens automatically ✅

> 💡 Moving the device to a new location? Simply power it on — if it can't find the saved network the portal opens automatically. No manual reset required.

---

## Configuration

Open **`variableInput.h`** and fill in your credentials and settings:

```cpp
// Static IP Configuration — must match router DHCP reservation
IPAddress local_IP(192, 168, 1, XXX);    // Your reserved IP for ESP32-S3
IPAddress gateway(192, 168, 1, 1);       // Your router gateway
IPAddress subnet(255, 255, 255, 0);      // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);        // Primary DNS
IPAddress secondaryDNS(8, 8, 4, 4);      // Secondary DNS

// Email Alert Settings
// Use a dedicated Google secondary account — not your primary Gmail
#define SMTP_HOST        "smtp.gmail.com"
#define SMTP_PORT        465
#define EMAIL_SENDER     "your_secondary@gmail.com"
#define EMAIL_PASSWORD   "your_app_password"
#define EMAIL_RECIPIENT  "alert_recipient@email.com"

// SMS / Text Alert Settings (via email-to-SMS gateway)
#define SMS_GATEWAY      "your_number@carrier_gateway.com"

// SinricPro
#define SINRICPRO_APP_KEY    "your_sinricpro_app_key"
#define SINRICPRO_APP_SECRET "your_sinricpro_app_secret"
#define FLOODING_DEVICE_ID   "your_flooding_device_id"
#define HIGHWATER_DEVICE_ID  "your_highwater_device_id"
#define ALLCLEAR_DEVICE_ID   "your_allclear_device_id"

// InfluxDB
#define INFLUXDB_URL     "http://your_pi_ip:8086"
#define INFLUXDB_TOKEN   "your_influxdb_token"
#define INFLUXDB_ORG     "your_org"
#define INFLUXDB_BUCKET  "your_bucket"

// Sensor Thresholds
#define TRIGGER_PIN      XX    // GPIO pin → sensor TRIG
#define ECHO_PIN         XX    // GPIO pin → sensor ECHO (via voltage divider)
#define HIGHWATER_CM     XX    // Distance (cm) that triggers HIGHWATER
#define FLOODING_CM      XX    // Distance (cm) that triggers FLOODING
```

> **Static IP & DHCP Reservation — Required:**
> The ESP32-S3 must have a **fixed IP address** — the Grafana iframe URLs embedded
> in the web interface HTML depend on it. Configure two things:
> 1. **Router DHCP reservation** — assign a fixed IP to the ESP32-S3 MAC address
>    in your router's admin panel so the router always gives it the same address.
> 2. **variableInput.h static IP** — set the same IP in the static IP configuration
>    above so the ESP32-S3 requests that address on boot.
> Both must match exactly for reliable operation.

> **Gmail users:** A standard Gmail password will not work — generate a dedicated
> **App Password** for this project.
> A dedicated **Google secondary account** is strongly recommended to keep
> project alerts separate from your primary inbox.

---

## SMS via Email-to-Text Gateways

| Carrier | SMS Gateway | Status |
| --- | --- | --- |
| AT&T | ~~`number@txt.att.net`~~ | **Shut down 2025** |
| Verizon | `number@vtext.com` | Active |
| T-Mobile | `number@tmomail.net` | Active |
| Boost Mobile | `number@sms.myboostmobile.com` | Active |
| Metro by T-Mobile | `number@mymetropcs.com` | Active |
| Google Fi | `number@msg.fi.google.com` | Active |
| US Cellular | `number@email.uscc.net` | Active |
| Cricket | ~~No longer available~~ | AT&T subsidiary |

> ⚠️ Email-to-SMS gateways are increasingly deprecated by carriers.
> Verify your carrier's current gateway if messages are not delivering.

---

## Wiring

| Sensor Pin | ESP32-S3 |
| --- | --- |
| VCC | 5V |
| GND | GND |
| TRIG | GPIO (defined by `TRIGGER_PIN`) |
| ECHO | Via voltage divider → GPIO (defined by `ECHO_PIN`) |

> ⚠️ **Voltage Divider Required on ECHO pin:**
> The ESP32-S3 GPIO pins are 3.3V logic only and are **NOT 5V tolerant.**
> The JSN-SR04T is a 5V device — its ECHO signal will damage the ESP32-S3 if
> connected directly. A resistor voltage divider is required:
>
> ```
> SENSOR ECHO ──┬── 1kΩ ──── ESP32-S3 ECHO PIN
>               │
>              2kΩ
>               │
>             GND
> ```
>
> This 1kΩ / 2kΩ divider reduces 5V → ~3.3V.
> The TRIG output from the ESP32-S3 at 3.3V is sufficient to trigger the JSN-SR04T
> and does **not** require level shifting.

---

## Calming Well Design

This project uses a **calming well** constructed from a section of 3" PVC pipe to eliminate false echoes from pit walls, pump hardware, and water surface turbulence:

* **Bottom cap** — drilled with inflow holes allowing pit water to freely equalize while dampening surface agitation
* **Top cap** — transducer mounted centrally, facing downward, firing directly down the bore toward the water surface

The calming well provides a clean, stable column of water with a flat reflective surface for consistent, accurate readings. The enclosed tube also shields the transducer from splash, condensation drip, and side-wall reflections.

---

## How It Works

1. On boot, the ESP32-S3 connects to WiFi and starts the AsyncWebServer
2. The JSN-SR04T fires a 40kHz pulse at configured intervals and measures echo return time
3. Distance to the water surface is calculated and compared against thresholds
4. Events are written to **InfluxDB** on the Raspberry Pi for historical tracking
5. Grafana panels on the Pi are **iframed** into the ESP32-S3 `/graph` web page
6. If water reaches **HIGHWATER** threshold — SinricPro triggers Alexa HIGHWATER routine
7. If water reaches **FLOODING** threshold — SinricPro triggers Alexa FLOODING routine; Alexa announces the alert and plays three Claxion alarm blasts via My Media
8. Simultaneously, SMS and email alerts are dispatched
9. All events and readings are logged to **LittleFS** `.txt` files
10. Logs are browsable via `/SdBrowse` and viewable via the built-in File Reader
11. LittleFS logs are automatically purged every **Sunday at midnight**

---

## Remote Access

This project uses [Tailscale Funnel](https://tailscale.com/kb/1223/funnel) on the Raspberry Pi to provide secure remote access to the Grafana dashboard without opening firewall ports or configuring dynamic DNS.

**Enable Tailscale Funnel on the Raspberry Pi:**
```bash
tailscale funnel --bg 80
```

Tailscale Funnel proxies external HTTPS traffic to Grafana running locally on port 3000:
```
https://your-pi-name.tailnet-name.ts.net  →  http://127.0.0.1:3000
```

This gives you secure, authenticated access to your Grafana dashboard from anywhere — no Caddy, no port forwarding, no dynamic DNS required.

---

## Related Projects

* [ESP-Now-with-Remote-Relay](https://github.com/Tech500/ESP-Now-with-Remote-Relay) — Extends this project with ESP-Now wireless relay switching for an auxiliary sump pump, Thingspeak graphing, FTP, OTA updates, and data logging

---

## License

This project is licensed under the [MIT License](https://github.com/Tech500/Sump-Monitor/blob/main/LICENSE).

---

## Credits

* **William Lucid (Tech500), AB9NQ** — Project author, hardware design, firmware development
* **Claude (Anthropic)** — AI development partner; firmware guidance, system architecture, and documentation

---

## Author

**William Lucid (Tech500), AB9NQ**
[GitHub Profile](https://github.com/Tech500)

---

> ⚠️ **Disclaimer:** This project is a passive monitoring aid. It is **not** a replacement
> for a properly maintained sump pump system, a battery backup pump, or professional
> flood protection. Always maintain your sump pump hardware and consider a secondary
> backup pump on a dedicated breaker.
