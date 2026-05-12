# Sump-Monitor
### Raspberry Pi 5 + ESP32-S3 Sump Pit Monitoring System
**Version 10**

Tech500/Sump-Monitor is a sump monitoring project with demo and live modes controlled in variableInput.h. Demo mode runs without the ultrasonic sensor, while live mode uses the waterproof JSN-SR04T ultrasonic sensor wired directly to the development board with Vcc, Ground, Trigger, and Echo connections. The flooding, high-water, and all-clear thresholds are also configurable in variableInput.h, which keeps setup straightforward and avoids digging through the codebase for values. The README already covers the supported hardware and configuration clearly, including the ESP32-ESP-1-N16R8 requirement and the 3.3 V sensor wiring.

---

## Project Structure

```
Sump-Monitor/
├── README.md
├── Claxion-3.mp3
├── LICENSE
├── Alexa/
│   ├── Alexa --Routines.pdf
│   └── Setup notes
├── Docs/
│   ├── Alexa_Sump_Monitor_Setup_Guide.pdf
│   ├── Full Startup Sequence.md
│   ├── JSN-SR04T-Datasheet.pdf
│   └── MOUNTING.md
├── ESP32-S3/
├── Raspberry Pi/
│   ├── Caddyfile
│   ├── GRAFANA_SETUP.md
│   ├── TAILSCALE_SETUP.md
│   ├── docker-compose.yml
│   ├── main.py
│   └── requirements.txt
├── Sump_Monitor/
│   └── variableInput.h
└── Video/
```

---

## ESP32 Configuration

All configurable settings are located in a single file:

```
Sump_Monitor/variableInput.h
```

Open this file and enter your specific values before flashing the ESP32-S3 (N16R8).

Settings include:
- SinricPro App Key, App Secret, and Device IDs (FLOODING, HIGHWATER, ALLCLEAR)
- InfluxDB endpoint and database name
- NTP server
- Pit threshold values (flooding distance, high water distance)
- Alert email / SMS settings

> **Note:** All values requiring your personal configuration are marked with `#########` placeholders.

---

## WiFi Setup — WifiManager

WiFi credentials are configured on first boot via WifiManager.

1. On first boot the ESP32 broadcasts as Access Point **Sump-Monitor**
2. Connect to **Sump-Monitor** AP from your phone or laptop
3. If **Sump-Monitor** does not appear, disconnect your device from your home network
4. Open browser and navigate to **192.168.4.1**
5. WiFi portal will display — select your WiFi network
   - 5.1 Select home network SSID
   - 5.2 Enter home network password
6. Watch address bar for confirmation that credentials are saved
7. Reconnect your device to your home network
8. Open Serial Monitor — confirm ESP32 shows SSID connected

---

## 1. Hardware Confirmation

### Confirm NVMe Drive

```bash
lsblk                           # Is drive visible?
ls /dev/nvme*                   # Device node exists?
sudo nvme list                  # Full NVMe details
sudo fdisk -l /dev/nvme0n1      # Disk info
```

### PCIe & NVMe dmesg

```bash
dmesg -T | grep -iE "pcie|nvme"
```

### What you want to see

```
nvme 0000:01:00.0: enabling device
nvme nvme0: Identified ctrl
nvme nvme0: 4/0/0 default/read/poll queues
```

### Install Utilities (if missing)

```bash
sudo apt update && sudo apt install nvme-cli lsof -y
```

> `nvme-cli` — NVMe drive management
> `lsof` — list open ports (not installed by default on Raspberry Pi OS)
> Install both early — they are needed in later steps.

---

## 2. Docker

### Install Docker

```bash
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker
```

### Install Docker Compose

```bash
sudo apt install docker-compose-plugin -y
docker compose version          # confirm install
```

### Start Stack

```bash
cd "Raspberry Pi"
docker compose up -d
```

### Stop Stack

```bash
docker compose down
```

### Restart Stack

```bash
docker compose restart
```

### View All Logs

```bash
docker compose logs -f
```

### View Logs Per Service

```bash
docker compose logs -f influxdb
docker compose logs -f fastapi
docker compose logs -f grafana
docker compose logs -f caddy
```

### Check Running Containers

```bash
docker ps
```

### Rebuild After Changes

```bash
docker compose down && docker compose up -d --build
```

---

## 3. FastAPI Endpoints

| Method | Endpoint | Description          |
|--------|----------|----------------------|
| POST   | /event   | Submit sump event    |
| GET    | /health  | Check API + InfluxDB |
| GET    | /recent  | Last 20 events       |

### Health Check

```bash
curl http://localhost:8001/health
```

### Recent Events

```bash
curl http://localhost:8001/recent
curl http://localhost:8001/recent?limit=50
```

### Post Test Event

```bash
curl -X POST http://localhost:8001/event \
  -H "Content-Type: application/json" \
  -d '{"type": "allclear", "location": "sump_pit", "distance": 12.5, "pitActivity": 0.0}'
```

### Valid Event Types

```
flooding
highwater
allclear
```

> **Note:** Dependencies are installed at container startup via the `command` directive
> in `docker-compose.yml`. `requirements.txt` is provided as a reference.

---

## 4. InfluxDB

### Access

```
URL:      http://localhost:8086
Database: sumpmonitor
User:     sump
Password: changeme123  ← change this!
```

### Open InfluxDB Shell

```bash
docker exec -it sump_influxdb influx
```

### Useful Queries

```sql
-- Select database
USE sumpmonitor;

-- Recent events
SELECT * FROM sump_events ORDER BY time DESC LIMIT 20;

-- Filter by event type
SELECT * FROM sump_events WHERE event_type='flooding';

-- Events in last hour
SELECT * FROM sump_events WHERE time > now() - 1h;

-- Events in last 24 hours
SELECT * FROM sump_events WHERE time > now() - 24h;

-- Count events by type
SELECT COUNT(value) FROM sump_events GROUP BY event_type;
```

---

## 5. Grafana

### Access

```
URL:            http://192.168.12.122/grafana/
Admin user:     admin
Admin password: changeme123  ← change this!
```

### Connect InfluxDB Datasource

1. Go to **Configuration → Data Sources**
2. Add **InfluxDB**
3. Set the following:
   ```
   URL:      http://influxdb:8086
   Database: sumpmonitor
   User:     sump
   Password: changeme123
   ```
4. Click **Save & Test**

> **Grafana v11.5.3 — Anonymous Access & Embedding**
> Grafana 10+ blocked anonymous access by default. This stack restores it
> via environment variables in `docker-compose.yml`. Do not remove the
> `GF_AUTH_ANONYMOUS_ENABLED` and `GF_AUTH_ANONYMOUS_ORG_ROLE` variables
> or dashboard access will require login. `GF_SECURITY_ALLOW_EMBEDDING=true`
> enables the dashboard to be embedded in the ESP32 web interface.
> Grafana is served from the `/grafana/` sub-path via Caddy reverse proxy.

> For detailed Grafana dashboard setup see: [`Raspberry Pi/GRAFANA_SETUP.md`](Raspberry%20Pi/GRAFANA_SETUP.md)

---

## 6. Caddy — Reverse Proxy (HTTP)

Caddy runs as a Docker container and reverse-proxies Grafana on the local network
via HTTP on port 80. A `Caddyfile` in the `Raspberry Pi/` directory controls routing
and is mounted into the container at startup.

> Caddy in this stack is a **local network proxy only** — port 80, no HTTPS.
> Tailscale provides secure remote access independently (see Section 7).

### Caddyfile

The `Caddyfile` is located at `Raspberry Pi/Caddyfile` and mounted into the
Caddy container via `docker-compose.yml`.

### Caddy Logs

```bash
docker compose logs -f caddy
```

### Confirm Port 80 Is Free Before Starting

```bash
sudo lsof -i :80

# Alternative — no install required:
sudo ss -tlnp | grep 80
```

### Confirm Caddy Is Running

```bash
docker exec -it sump_caddy caddy version
```

---

## 7. Tailscale Remote Access

### Install Tailscale

```bash
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up
```

### Check Tailscale Status

```bash
tailscale status
```

### Remote Access URLs

| Service  | URL                                  |
|----------|--------------------------------------|
| Grafana  | http://192.168.12.122/grafana/       |
| FastAPI  | http://\:8001                |
| InfluxDB | http://\:8086                |

> For detailed Tailscale configuration see: [`Raspberry Pi/TAILSCALE_SETUP.md`](Raspberry%20Pi/TAILSCALE_SETUP.md)

---

## 8. Alexa & SinricPro Integration

When a flooding event is detected, the ESP32 triggers a SinricPro virtual contact
sensor. Alexa responds by playing the Claxion-3 alarm sound across all Echo devices
and announcing a voice alert.

### How It Works

```
ESP32 detects flooding
  → Posts event to FastAPI
  → Triggers SinricPro FLOODING contact sensor (open)
    → Alexa FLOODING routine fires:
        1. Set volume to 5
        2. Play Claxion-3.mp3 via My Media
        3. Announce "Sump pit flooding take immediate action"
           (broadcast to all 4 Echo devices simultaneously)
```

### Virtual Sensors (SinricPro)

| Sensor Name | Type           | Trigger Condition      |
|-------------|----------------|------------------------|
| FLOODING    | Contact Sensor | Water at flood level   |
| HIGHWATER   | Contact Sensor | Water at high level    |
| ALLCLEAR    | Contact Sensor | Water returned to safe |

- SinricPro free plan covers all 3 sensors — no payment required
- Get your App Key, App Secret, and Device IDs from [sinric.pro](https://sinric.pro)
- Enter these values in `Sump_Monitor/variableInput.h` before flashing

### Claxion-3 Alarm Sound

`Claxion-3.mp3` (repo root) is the alarm audio played by Alexa via the **My Media**
skill. It must be placed in your My Media watch folder on the Raspberry Pi and indexed
before the routine will work.

My Media runs separately on the Raspberry Pi (not part of the Docker Compose stack).
For installation see: [https://www.mymediaalexa.com/home/raspberrypi](https://www.mymediaalexa.com/home/raspberrypi)

### Required Alexa Skills

Two skills must be enabled in the Alexa app before setup:
- **SinricPro** — links virtual contact sensors to Alexa (search Alexa+ Store)
- **My Media** — enables local playback of Claxion-3.mp3 from the Pi (search Alexa+ Store → Original Skills)

### Full Setup Guide

Complete step-by-step instructions with screenshots covering SinricPro account
creation, virtual sensor setup, Alexa skill enabling, device discovery, and
routine creation:

📄 [`Docs/Alexa_Sump_Monitor_Setup_Guide.pdf`](Docs/Alexa_Sump_Monitor_Setup_Guide.pdf)

---

## 9. NVMe Mount (if needed)

### Format NVMe — first time only!

```bash
sudo mkfs.ext4 /dev/nvme0n1
```

### Create Mount Point

```bash
sudo mkdir -p /mnt/nvme
```

### Mount NVMe

```bash
sudo mount /dev/nvme0n1 /mnt/nvme
```

### Auto Mount on Boot

```bash
echo '/dev/nvme0n1 /mnt/nvme ext4 defaults 0 2' | sudo tee -a /etc/fstab
```

### Verify Mount

```bash
df -h | grep nvme
```

---

## 10. Troubleshooting

| Symptom                              | Check                                                                              |
|--------------------------------------|------------------------------------------------------------------------------------|
| NVMe not showing in lsblk            | Reseat ribbon cable, check dmesg                                                   |
| PCIe visible but NVMe silent         | Ribbon cable not fully seated                                                      |
| FastAPI container crashing           | `docker compose logs -f fastapi`                                                   |
| InfluxDB connection refused          | Wait for container to fully start                                                  |
| Grafana no data                      | Verify datasource URL uses `influxdb:8086`                                         |
| Grafana login required               | Confirm `GF_AUTH_ANONYMOUS_ENABLED=true` in `docker-compose.yml`                  |
| Docker permission denied             | `sudo usermod -aG docker $USER`                                                    |
| Port already in use                  | `sudo lsof -i :` or `sudo ss -tlnp | grep `                           |
| `lsof` command not found             | `sudo apt install lsof -y` — or use `sudo ss -tlnp | grep ` (built-in)      |
| Caddy not serving on port 80         | Confirm port 80 is free: `sudo ss -tlnp | grep 80`                                |
| Grafana not reachable via /grafana/  | Check Caddyfile routing and confirm `GF_SERVER_SERVE_FROM_SUB_PATH=true`          |
| Alexa routine not triggering         | Verify SinricPro skill enabled; FLOODING shows Online (green) in SinricPro        |
| Claxion-3 not playing                | Confirm My Media is running on Pi and file is indexed                              |

---

## 11. Pi 5 NVMe HAT — config.txt

If NVMe is not detected, add to `/boot/firmware/config.txt`:

```
dtparam=pciex1
dtparam=pciex1_gen=3
```

Then reboot:

```bash
sudo reboot
```

---

## 12. Quick Reference — Full Startup Sequence

For the complete annotated startup sequence see: [`Docs/Full Startup Sequence.md`](Docs/Full%20Startup%20Sequence.md)

```bash
# ── STEP 1: OS Update ────────────────────────────────────────────────────────
sudo apt update && sudo apt upgrade -y
sudo reboot

# ── STEP 2: Confirm NVMe ─────────────────────────────────────────────────────
lsblk
dmesg -T | grep -iE "pcie|nvme"

# ── STEP 3: Install Utilities ────────────────────────────────────────────────
sudo apt update && sudo apt install nvme-cli lsof git -y
# nvme-cli  — NVMe management
# lsof      — port checking (not installed by default on Raspberry Pi OS)
# git       — clone the repo
sudo nvme list

# ── STEP 4: Install Docker ───────────────────────────────────────────────────
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker

# ── STEP 5: Install Docker Compose ───────────────────────────────────────────
sudo apt install docker-compose-plugin -y
docker compose version

# ── STEP 6: Install Tailscale ────────────────────────────────────────────────
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up

# ── STEP 7: Confirm Ports Are Free ───────────────────────────────────────────
sudo lsof -i :8086   # InfluxDB
sudo lsof -i :8001   # FastAPI
sudo lsof -i :3000   # Grafana
sudo lsof -i :80     # Caddy HTTP

# Alternative if lsof unavailable:
sudo ss -tlnp | grep -E '8086|8001|3000|80'

# ── STEP 8: Clone Project ────────────────────────────────────────────────────
git clone https://github.com/Tech500/Sump-Monitor
cd Sump-Monitor

# ── STEP 8a: Configure ESP32 ─────────────────────────────────────────────────
# Edit variableInput.h with your specific settings before flashing ESP32
# All configurable values are located in this single file
nano Sump_Monitor/variableInput.h

# ── STEP 9: Create Directories ───────────────────────────────────────────────
mkdir -p app
mkdir -p mymedia

# ── STEP 10: Confirm NVMe Mounted ────────────────────────────────────────────
df -h | grep nvme

# ── STEP 11: Start Stack ─────────────────────────────────────────────────────
cd "Raspberry Pi"
docker compose up -d
docker ps

# ── STEP 12: Confirm Health ──────────────────────────────────────────────────
curl http://localhost:8001/health

# ── STEP 13: Open Grafana ────────────────────────────────────────────────────
# http://192.168.12.122/grafana/
# admin / changeme123  ← change this!

# ── STEP 14: Post Test Event ─────────────────────────────────────────────────
curl -X POST http://localhost:8001/event \
  -H "Content-Type: application/json" \
  -d '{"type": "allclear", "location": "sump_pit", "distance": 12.5, "pitActivity": 0.0}'

# ── STEP 15: Confirm Event in InfluxDB ───────────────────────────────────────
docker exec -it sump_influxdb influx -execute \
  'SELECT * FROM sump_events ORDER BY time DESC LIMIT 5' \
  -database sumpmonitor
```

---

## Documentation Index

| File | Description |
|------|-------------|
| [`Docs/Alexa_Sump_Monitor_Setup_Guide.pdf`](Docs/Alexa_Sump_Monitor_Setup_Guide.pdf) | Complete Alexa & SinricPro setup with screenshots |
| [`Docs/Full Startup Sequence.md`](Docs/Full%20Startup%20Sequence.md) | Annotated full startup sequence |
| [`Docs/MOUNTING.md`](Docs/MOUNTING.md) | JSN-SR04T sensor mounting instructions |
| [`Docs/JSN-SR04T-Datasheet.pdf`](Docs/JSN-SR04T-Datasheet.pdf) | Ultrasonic sensor datasheet |
| [`Raspberry Pi/GRAFANA_SETUP.md`](Raspberry%20Pi/GRAFANA_SETUP.md) | Grafana dashboard configuration |
| [`Raspberry Pi/TAILSCALE_SETUP.md`](Raspberry%20Pi/TAILSCALE_SETUP.md) | Tailscale remote access setup |
| [`Alexa/Alexa --Routines.pdf`](Alexa/Alexa%20--Routines.pdf) | Alexa routine reference |

---

## Acknowledgments

Project Development Assistant and README documentation for Version 10 developed with assistance from
[Claude](https://claude.ai) by Anthropic.
