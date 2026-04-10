# Sump-Monitor

**Sump pit monitor using an ultrasonic transducer with text and email alerts.**

An ESP32-based IoT project that continuously monitors the water level in a basement sump pit using an ultrasonic distance sensor. When the water level rises to a configured threshold, the system sends both SMS text and email alert notifications to keep you informed of potential pump failures or flooding risks — before a problem becomes a disaster.

---

## Features

- **Ultrasonic water level sensing** — Uses an ultrasonic transducer (e.g., JSN-SR04T or HC-SR04) to measure distance to the water surface, calculating the current pit water level
- **SMS text alerts** — Sends text message notifications when water level exceeds the configured alarm threshold
- **Email alerts** — Sends email notifications simultaneously with text alerts for redundant notification coverage
- **Built-in web interface** — Local network web server for real-time monitoring of current water level readings
- **Arduino IDE compatible** — Written in C/C++, programmed using the Arduino IDE with ESP32 board support

---

## Hardware Requirements

| Component | Description |
|---|---|
| ESP32 Development Board | e.g., ESP32 DevKit V1 or similar |
| Ultrasonic Transducer | JSN-SR04T (waterproof, recommended) or HC-SR04 |
| Power Supply | 5V USB or regulated power adapter |
| Enclosure | Weatherproof enclosure suitable for basement/pit environment |

> **Note:** The JSN-SR04T waterproof variant is strongly recommended over the HC-SR04 for sump pit environments due to the high humidity and potential water splash exposure.

---

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x)
- ESP32 board support package — Add the following URL to **File > Preferences > Additional Board Manager URLs**:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Required libraries (install via Arduino Library Manager):
  - `ESPAsyncWebServer`
  - `AsyncTCP`
  - `ESP Mail Client` (for email alerts)
  - Any additional libraries referenced in the sketch header

---

## Configuration

Open the configuration/variable input file (e.g., `variableInput.h` or the `#define` section at the top of the main `.ino` sketch) and fill in your credentials and settings:

```cpp
// WiFi Credentials
#define WIFI_SSID        "your_wifi_ssid"
#define WIFI_PASSWORD    "your_wifi_password"

// Email Alert Settings
#define SMTP_HOST        "smtp.gmail.com"         // or your SMTP server
#define SMTP_PORT        465
#define EMAIL_SENDER     "your_email@gmail.com"
#define EMAIL_PASSWORD   "your_app_password"
#define EMAIL_RECIPIENT  "alert_recipient@email.com"

// SMS / Text Alert Settings (via email-to-SMS gateway)
// Example: Verizon: 10digitnumber@vtext.com
#define SMS_GATEWAY      "your_number@carrier_gateway.com"

// Sensor & Alert Thresholds
#define TRIGGER_PIN      XX     // GPIO pin connected to sensor TRIG
#define ECHO_PIN         XX     // GPIO pin connected to sensor ECHO
#define ALARM_LEVEL_CM   XX     // Water level (cm) that triggers alert
```

> **Gmail users:** Use an [App Password](https://support.google.com/accounts/answer/185833) rather than your main account password.

---

## SMS via Email-to-Text Gateways

Most U.S. carriers support free email-to-SMS gateways. Send an email to the address below (replacing `number` with the 10-digit phone number):

| Carrier | Gateway Address |
|---|---|
| AT&T | `number@txt.att.net` |
| Verizon | `number@vtext.com` |
| T-Mobile | `number@tmomail.net` |
| Sprint | `number@messaging.sprintpcs.com` |
| US Cellular | `number@email.uscc.net` |

---

## Wiring

| Sensor Pin | ESP32 GPIO |
|---|---|
| VCC | 5V |
| GND | GND |
| TRIG | Defined by `TRIGGER_PIN` |
| ECHO | Via voltage divider → Defined by `ECHO_PIN` |

> ⚠️ **IMPORTANT — Voltage Divider Required on ECHO pin:**  
> The ESP32-S3 GPIO pins are **3.3V logic only and are NOT 5V tolerant.** If your ultrasonic transducer operates at 5V (e.g., HC-SR04), the ECHO signal it returns will also be 5V — which **will damage the ESP32-S3** if connected directly.  
>
> A simple resistor voltage divider on the ECHO line brings the signal down to a safe 3.3V level:
>
> ```
> SENSOR ECHO ──┬── 1kΩ ──── ESP32 ECHO PIN
>               │
>              2kΩ
>               │
>             GND
> ```
>
> This 1kΩ / 2kΩ divider reduces 5V → ~3.3V. Other resistor pairs with a 1:2 ratio (e.g., 10kΩ / 20kΩ) work equally well.  
> The TRIG pin output from the ESP32-S3 at 3.3V is sufficient to trigger most 5V ultrasonic sensors and does **not** require level shifting.  
> The JSN-SR04T waterproof variant is available in a 3.3V version, which connects directly with no voltage divider needed.

> **Sensor Mounting — Calming Well Design:**  
> This project uses a **calming well** constructed from a section of 3" PVC pipe to eliminate false echoes caused by pit wall reflections, pump hardware, and water surface turbulence.
>
> - **Bottom cap** — drilled with inflow holes to allow pit water to freely enter and equalize with the pit level while dampening surface agitation
> - **Top cap** — transducer mounted centrally in the end cap, facing downward, firing directly down the bore of the pipe toward the water surface
>
> The calming well provides a clean, stable column of water with a flat reflective surface, resulting in consistent and accurate distance readings. The enclosed tube also shields the transducer face from splash, condensation drip, and side-wall reflections that commonly cause erratic readings when sensors are mounted openly above the pit.

---

## How It Works

1. On boot, the ESP32 connects to your WiFi network and starts the web server.
2. The ultrasonic sensor fires a 40 kHz pulse at a configured interval and measures the echo return time.
3. Distance to the water surface is calculated using the speed of sound and converted to a water level reading.
4. If the measured water level reaches or exceeds `ALARM_LEVEL_CM`, an alert email (and SMS via email-to-text gateway) is sent.
5. Alerts are rate-limited to prevent inbox flooding during sustained high-water events.
6. Current readings are accessible via the built-in web interface on the local network.

---

## Web Interface

Once connected to WiFi, the device hosts a web server accessible at its local IP address (check your router's DHCP table or the Serial Monitor output on first boot):

```
http://<ESP32_IP_ADDRESS>/
```

The interface displays the current water level reading and system status.

---

## Remote Access

The built-in web interface is available on your local network by default. For **secure remote access from anywhere** — without opening ports or relying on dynamic DNS — [Tailscale](https://tailscale.com) combined with [Caddy](https://caddyserver.com) provides an elegant, zero-trust solution.

This Hackster.io guide by AB9NQ walks through the full setup for securely exposing ESP32 web dashboards over Tailscale with Caddy as an HTTPS reverse proxy:

> 📖 [**Secure Weather Dashboards via Tailscale and Caddy**](https://www.hackster.io/AB9NQ/secure-weather-dashboards-via-tailscale-and-caddy-94c424)

The same approach applies directly to the Sump-Monitor web interface — since this project runs continuously, a **Raspberry Pi is the ideal host** for Tailscale and Caddy. It draws minimal power, runs 24/7 without issue, and sits quietly on your local network proxying requests to the ESP32. This gives you encrypted, authenticated access to your sump level readings from any device on your Tailscale network, around the clock.

---

## Related Projects

- [ESP-Now-with-Remote-Relay](https://github.com/Tech500/ESP-Now-with-Remote-Relay) — Extends this project with ESP-Now wireless relay switching for an auxiliary sump pump, Thingspeak graphing, FTP, OTA updates, and data logging at 15-minute intervals.

---

## License

This project is licensed under the [MIT License](LICENSE).

---

## Author

**William Lucid (Tech500)**  
[GitHub Profile](https://github.com/Tech500)

---

> ⚠️ **Disclaimer:** This project is a passive monitoring aid. It is **not** a replacement for a properly maintained sump pump system, a battery backup pump, or professional flood protection. Always maintain your sump pump hardware and consider a secondary backup pump on a dedicated breaker.
