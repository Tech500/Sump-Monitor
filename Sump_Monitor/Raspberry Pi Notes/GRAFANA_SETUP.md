# Grafana Setup — Sump Monitor

This guide covers installing and configuring Grafana to visualize data from the Sump Monitor project. The ESP32-S3 POSTs JSON events to a small listener on the Pi, which writes them to InfluxDB. Grafana reads from InfluxDB to display water distance and alert history.

---

## Device Architecture

This project runs across two devices on the local network:

| Device | IP | Role |
|---|---|---|
| ESP32-S3 | 192.168.12.22 | AsyncWebServer, web UI, index3.h iframes, sendToGrafana() |
| Raspberry Pi | 192.168.12.146 | Docker, InfluxDB, Grafana, sump-listener |

Both devices are members of the same **Tailscale tailnet**, which means Tailscale subnet routing transparently handles traffic between them — and to any other device invited to the tailnet — regardless of which network they are physically on.

---

## Data Flow Overview

```
ESP32-S3 (192.168.12.22)
  │
  │  HTTP POST  http://192.168.12.146:8001/event
  │  JSON: { "type": "allclear|highwater|flooding",
  │           "location": "sump_pit",
  │           "distance": 4.2 }
  ▼
Event Listener on Pi (port 8001)
  │
  │  writes measurement
  ▼
InfluxDB  ──────────────────►  Grafana (port 3000)
  │
  └── iframes embedded in ESP32 web UI (index3.h)
```

Events are sent on every **condition change** and polled every **5 minutes**.

---

## Prerequisites

- Raspberry Pi running Raspberry Pi OS (Bullseye or Bookworm)
- Docker and Docker Compose installed
- The Sump Monitor sketch flashed and running on the ESP32-S3
- Both devices on the same LAN and Tailscale tailnet

---

## Docker Compose Setup

Create a `docker-compose.yml` on the Pi (`192.168.12.146`):

```yaml
version: "3.8"

services:

  influxdb:
    image: influxdb:1.8
    container_name: influxdb
    restart: unless-stopped
    ports:
      - "8086:8086"
    volumes:
      - influxdb-data:/var/lib/influxdb
    environment:
      - INFLUXDB_DB=sump_monitor
      - INFLUXDB_HTTP_AUTH_ENABLED=false
    networks:
      - monitoring

  event-listener:
    image: python:3.11-slim
    container_name: sump-listener
    restart: unless-stopped
    ports:
      - "8001:8001"
    volumes:
      - ./listener.py:/app/listener.py
    working_dir: /app
    command: sh -c "pip install influxdb -q && python listener.py"
    depends_on:
      - influxdb
    networks:
      - monitoring

  grafana:
    image: grafana/grafana:latest
    container_name: grafana
    restart: unless-stopped
    ports:
      - "3000:3000"
    volumes:
      - grafana-storage:/var/lib/grafana
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=changeme       # Change this!
      - GF_SECURITY_ALLOW_EMBEDDING=true
      - GF_AUTH_ANONYMOUS_ENABLED=true
      - GF_AUTH_ANONYMOUS_ORG_ROLE=Viewer
    depends_on:
      - influxdb
    networks:
      - monitoring

volumes:
  influxdb-data:
  grafana-storage:

networks:
  monitoring:
    driver: bridge
```

---

## Event Listener Script

Create `listener.py` in the same directory as `docker-compose.yml`:

```python
from http.server import BaseHTTPRequestHandler, HTTPServer
from influxdb import InfluxDBClient
import json

client = InfluxDBClient(host="influxdb", port=8086, database="sump_monitor")

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == "/event":
            length = int(self.headers.get("Content-Length", 0))
            body = json.loads(self.rfile.read(length))

            point = [{
                "measurement": "sump_event",
                "tags": {
                    "location": body.get("location", "sump_pit"),
                    "type":     body.get("type", "unknown")
                },
                "fields": {
                    "distance": float(body.get("distance", 0.0))
                }
            }]

            client.write_points(point)
            print(f"[listener] wrote: {body}")

            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"OK")
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, format, *args):
        pass   # suppress default access log noise

HTTPServer(("0.0.0.0", 8001), Handler).serve_forever()
```

---

## Start the Stack

```bash
docker compose up -d
```

Verify all three containers are running:

```bash
docker ps
```

Check listener logs to confirm it's receiving POSTs from the ESP32:

```bash
docker logs sump-listener
```

---

## First Login to Grafana

1. Open `http://192.168.12.146:3000` in a browser
2. Log in with your admin credentials
3. Anonymous viewer access is enabled — shared users see dashboards without logging in

---

## Add InfluxDB as a Data Source

1. Go to **Connections → Data Sources → Add data source**
2. Select **InfluxDB**
3. Fill in:

| Field    | Value                  |
|----------|------------------------|
| URL      | `http://influxdb:8086` |
| Database | `sump_monitor`         |
| Auth     | leave disabled         |

4. Click **Save & Test**

---

## Dashboard Panels

The Sump Monitor dashboard consists of five panels:

| Panel | Type | Description |
|---|---|---|
| All Events | Time series | Flooding and highwater events over time |
| Flooding Events | Bar chart | Each flooding condition, red |
| High Water Events | Bar chart | Each high water condition, yellow |
| All Clear Events | Bar chart | Each all clear condition, green |
| Pit Activity — Distance To Target | Time series | Raw distance readings in inches |

Panel embed URLs are generated by Grafana (**Share → Embed**) and pasted directly into `index3.h` on the ESP32 as iframes, pointing to `http://192.168.12.146:3000`.

---

## Keeping the Stack Updated

```bash
docker compose pull
docker compose up -d
```

---

## Troubleshooting

| Symptom | What to check |
|---|---|
| Grafana iframes blank | Confirm `GF_SECURITY_ALLOW_EMBEDDING=true` is set |
| No data in panels | `docker logs sump-listener` — check for write errors |
| InfluxDB data source test fails | Confirm both containers share the `monitoring` network |
| ESP32 POST fails | Confirm Pi is reachable at `192.168.12.146` and listener is running |
| Forgot Grafana password | `docker exec -it grafana grafana-cli admin reset-admin-password <newpassword>` |
