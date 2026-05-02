# Sump-Monitor
### Raspberry Pi 5 + ESP32-S3 Sump Pit Monitoring System

---

## Project Structure

```
sump-monitor/
├── README.md
├── docker-compose.yml
├── app/
│   ├── main.py
│   └── requirements.txt
└── mymedia/
```

---

## ESP32 Configuration

All configurable settings are located in a single file:

```
variableInput.h
```

Open this file and enter your specific values before flashing the ESP32-S3 (N16R8).

Settings include:
- SinricPro keys
- InfluxDB endpoint
- NTP server
- Pit threshold values

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

### Install nvme-cli if missing

```bash
sudo apt update && sudo apt install nvme-cli -y
```

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
cd ~/sump-monitor
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

---

## 4. InfluxDB

### Access

```
URL:      http://localhost:8086
Database: sumpmonitor
User:     sump
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
URL:            http://<Pi-IP>:3000
Admin user:     admin
Admin password: changeme123  ← change this!
```

### Connect InfluxDB Datasource

1. Log in as admin first at `http://<Pi-IP>:3000/login`
2. Navigate directly to:
   ```
   http://<Pi-IP>:3000/connections/datasources/new
   ```
3. Select **InfluxDB**
4. Set the following:
   ```
   URL:      http://influxdb:8086   ← use container name, NOT localhost
   Database: sumpmonitor
   User:     sump
   Password: changeme123
   ```
5. Click **Save & Test** — confirm green ✅ datasource is working

> **Note:** The URL must use `influxdb` (Docker container name) not `localhost`.
> Grafana communicates with InfluxDB internally within the Docker network.

---

## 6. Tailscale Remote Access

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

| Service  | URL                            |
|----------|--------------------------------|
| Grafana  | http://<tailscale-ip>:3000     |
| FastAPI  | http://<tailscale-ip>:8001     |
| InfluxDB | http://<tailscale-ip>:8086     |

---

## 7. NVMe Mount (if needed)

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

## 8. Troubleshooting

| Symptom                        | Check                                      |
|--------------------------------|--------------------------------------------|
| NVMe not showing in lsblk      | Reseat ribbon cable, check dmesg           |
| PCIe visible but NVMe silent   | Ribbon cable not fully seated              |
| FastAPI container crashing     | `docker compose logs -f fastapi`           |
| InfluxDB connection refused    | Wait for container to fully start          |
| Grafana no data                | Verify datasource URL uses influxdb:8086   |
| Grafana datasource redirecting | Log in as admin first                      |
| Docker permission denied       | `sudo usermod -aG docker $USER`            |
| Port already in use            | `sudo lsof -i :<port>`                     |
| lsof command not found         | `sudo apt install lsof -y`                 |
| docker-compose not found       | Use `docker compose` (no hyphen)           |

---

## 9. Pi 5 NVMe HAT — config.txt

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

## 10. Quick Reference — Full Startup Sequence

```bash
# ── STEP 1: OS Update ────────────────────────────────────────────────────────
sudo apt update && sudo apt upgrade -y
sudo reboot

# ── STEP 2: Confirm NVMe ─────────────────────────────────────────────────────
lsblk
dmesg -T | grep -iE "pcie|nvme"
sudo apt install nvme-cli -y
sudo nvme list

# ── STEP 3: Install Required Tools ───────────────────────────────────────────
sudo apt install git lsof -y

# ── STEP 4: Install Docker ───────────────────────────────────────────────────
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker

# ── STEP 5: Install Docker Compose ───────────────────────────────────────────
sudo apt install docker-compose-plugin -y
docker compose version

# ── STEP 6: Confirm Ports Are Free ───────────────────────────────────────────
sudo lsof -i :8086   # InfluxDB
sudo lsof -i :8001   # FastAPI
sudo lsof -i :3000   # Grafana

# ── STEP 7: Clone Project ────────────────────────────────────────────────────
git clone https://github.com/Tech500/Sump-Monitor
cd Sump-Monitor/Raspberry\ Pi

# ── STEP 7a: Configure ESP32 ─────────────────────────────────────────────────
# Edit variableInput.h with your specific settings before flashing ESP32
# All configurable values are located in this single file
nano variableInput.h

# ── STEP 8: Create Directories ───────────────────────────────────────────────
mkdir -p app
mkdir -p mymedia

# ── STEP 8a: Copy main.py to app folder ──────────────────────────────────────
cp main.py app/

# ── STEP 9: Confirm NVMe Mounted ─────────────────────────────────────────────
df -h | grep nvme

# ── STEP 10: Start Stack ─────────────────────────────────────────────────────
docker compose up -d
docker ps

# ── STEP 11: Confirm Health ──────────────────────────────────────────────────
curl http://localhost:8001/health

# ── STEP 12: Open Grafana ────────────────────────────────────────────────────
# http://<Pi-IP>:3000
# Log in: admin / changeme123  ← change this!
# Navigate to: http://<Pi-IP>:3000/connections/datasources/new
# Add InfluxDB datasource:
#   URL:      http://influxdb:8086
#   Database: sumpmonitor
#   User:     sump
#   Password: changeme123

# ── STEP 13: Post Test Event ─────────────────────────────────────────────────
curl -X POST http://localhost:8001/event \
  -H "Content-Type: application/json" \
  -d '{"type": "allclear", "location": "sump_pit", "distance": 12.5, "pitActivity": 0.0}'

# ── STEP 14: Confirm Event in InfluxDB ───────────────────────────────────────
docker exec -it sump_influxdb influx -execute \
  'SELECT * FROM sump_events ORDER BY time DESC LIMIT 5' \
  -database sumpmonitor
```
