# Sump Monitor - Project Cheat Sheet

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

## 1. Hardware Confirmation

### Confirm NVMe Drive
```bash
lsblk                    # Is drive visible?
ls /dev/nvme*            # Device node exists?
sudo nvme list           # Full NVMe details
sudo fdisk -l /dev/nvme0n1  # Disk info
```

### PCIe & NVMe dmesg (one shot)
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
docker compose version   # confirm install
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
docker compose logs -f mymedia
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

| Method | Endpoint  | Description              |
|--------|-----------|--------------------------|
| POST   | /event    | Submit sump event        |
| GET    | /health   | Check API + InfluxDB     |
| GET    | /recent   | Last 20 events           |

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
- `flooding`
- `highwater`
- `allclear`

---

## 4. InfluxDB

### Access
- URL: `http://localhost:8086`
- Database: `sumpmonitor`
- User: `sump`

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
- URL: `http://localhost:3000`
- Admin user: `admin`
- Admin password: `changeme123` ← **change this!**

### Connect InfluxDB Datasource
1. Go to **Configuration → Data Sources**
2. Add **InfluxDB**
3. URL: `http://influxdb:8086`
4. Database: `sumpmonitor`
5. User: `sump`
6. Password: `changeme123`
7. Click **Save & Test**

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
| Service  | URL                                        |
|----------|--------------------------------------------|
| Grafana  | https://<your-tailscale-hostname>:3000     |
| FastAPI  | https://<your-tailscale-hostname>:8001     |
| InfluxDB | https://<your-tailscale-hostname>:8086     |

---

## 7. NVMe Mount (if needed)

### Format NVMe (first time only!)
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

| Symptom                          | Check                                      |
|----------------------------------|--------------------------------------------|
| NVMe not showing in `lsblk`      | Reseat ribbon cable, check `dmesg`         |
| PCIe visible but NVMe silent     | Ribbon cable not fully seated              |
| FastAPI container crashing       | Check `docker compose logs -f fastapi`     |
| InfluxDB connection refused      | Wait for container to fully start          |
| Grafana no data                  | Verify datasource URL uses `influxdb:8086` |
| Docker permission denied         | Run `sudo usermod -aG docker $USER`        |
| Port already in use              | Run `sudo lsof -i :<port>`                 |

---

## 9. Pi 5 NVMe HAT - config.txt

Add to `/boot/firmware/config.txt` if NVMe not detected:
```
dtparam=pciex1
dtparam=pciex1_gen=3
```

Then reboot:
```bash
sudo reboot
```

---

## 10. Quick Reference - Full Startup Sequence

```bash
# ── STEP 1: OS Update ────────────────────────────────────────────────────────
sudo apt update && sudo apt upgrade -y
sudo reboot

# ── STEP 2: Confirm NVMe ─────────────────────────────────────────────────────
lsblk
dmesg -T | grep -iE "pcie|nvme"
sudo apt install nvme-cli -y
sudo nvme list

# ── STEP 3: Install Git ──────────────────────────────────────────────────────
sudo apt install git -y

# ── STEP 4: Install Docker ───────────────────────────────────────────────────
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker

# ── STEP 5: Install Docker Compose ───────────────────────────────────────────
sudo apt install docker-compose-plugin -y
docker compose version   # confirm

# ── STEP 6: Confirm Ports Are Free ───────────────────────────────────────────
sudo lsof -i :8086   # InfluxDB
sudo lsof -i :8001   # FastAPI
sudo lsof -i :3000   # Grafana
sudo lsof -i :52051  # MyMedia

# ── STEP 7: Clone Project ────────────────────────────────────────────────────
git clone https://github.com/Tech500/Sump-Monitor
cd Sump-Monitor

# ── STEP 8: Create Directories ───────────────────────────────────────────────
mkdir -p app
mkdir -p mymedia

# ── STEP 9: Confirm NVMe Mounted ─────────────────────────────────────────────
df -h | grep nvme

# ── STEP 10: Start Stack ─────────────────────────────────────────────────────
docker compose up -d
docker ps   # confirm all containers running

# ── STEP 11: Confirm Health ──────────────────────────────────────────────────
curl http://localhost:8001/health

# ── STEP 12: Open Grafana ────────────────────────────────────────────────────
# http://localhost:3000
# admin / changeme123  ← change this!

# ── STEP 13: Post Test Event ─────────────────────────────────────────────────
curl -X POST http://localhost:8001/event \
  -H "Content-Type: application/json" \
  -d '{"type": "allclear", "location": "sump_pit", "distance": 12.5, "pitActivity": 0.0}'

# ── STEP 14: Confirm Event in InfluxDB ───────────────────────────────────────
docker exec -it sump_influxdb influx -execute \
  'SELECT * FROM sump_events ORDER BY time DESC LIMIT 5' \
  -database sumpmonitor
```