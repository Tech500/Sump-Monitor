# Tailscale Setup — Sump Monitor

This guide covers installing Tailscale on the Raspberry Pi and ESP32-accessible devices, enabling subnet routing so all tailnet members can reach the Sump Monitor web UI and Grafana dashboard from anywhere — no port forwarding, no dynamic DNS, no router changes required.

---

## Why Tailscale?

The Sump Monitor runs across two LAN devices:

| Device | IP | Role |
|---|---|---|
| ESP32-S3 | 192.168.12.22 | Web UI, AsyncWebServer |
| Raspberry Pi | 192.168.12.146 | Grafana, InfluxDB, event listener |

Without Tailscale, both devices are only reachable on the local LAN (`R2D2`). With Tailscale subnet routing, any invited device — phone, laptop, a friend's PC in another city — can reach both devices transparently using their normal LAN IPs (`192.168.12.22`, `192.168.12.146`), regardless of physical location or network.

```
Any tailnet device (anywhere)
  │
  │  Tailscale mesh (WireGuard encrypted)
  ▼
Pi subnet router (192.168.12.146)
  │
  ├── 192.168.12.22   → ESP32 web UI
  └── 192.168.12.146  → Grafana :3000, listener :8001
```

---

## Step 1 — Install Tailscale on the Pi

```bash
curl -fsSL https://tailscale.com/install.sh | sh
```

Start and authenticate:

```bash
sudo tailscale up --advertise-routes=192.168.12.0/24
```

Tailscale will print an authentication URL. Open it in a browser, log in to your Tailscale account, and approve the device.

The `--advertise-routes` flag tells Tailscale to advertise your home subnet (`192.168.12.0/24`) to all tailnet members, making both the ESP32 and Pi reachable by their LAN IPs from anywhere.

Confirm it's connected:

```bash
tailscale status
```

---

## Step 2 — Approve Subnet Routes in the Admin Console

1. Go to [https://login.tailscale.com/admin/machines](https://login.tailscale.com/admin/machines)
2. Find `lucidpi` in the machine list
3. Click the **three dots menu → Edit route settings**
4. Enable the `192.168.12.0/24` subnet route
5. Save

---

## Step 3 — Enable Funnel for the Web UI (port 80)

Tailscale Funnel exposes the ESP32 AsyncWebServer publicly so anyone with the URL can reach it without needing Tailscale installed:

```bash
sudo tailscale funnel --bg 80
```

Verify:

```bash
tailscale funnel status
```

Expected output:
```
https://lucidpi.tailb986d2.ts.net (Funnel on)
|-- / proxy http://127.0.0.1:80
```

The public URL for the Sump Monitor web UI is:
```
https://lucidpi.tailb986d2.ts.net
```

---

## Sharing Access

There are two ways to share access depending on who you're sharing with:

### Option A — Tailscale invite (recommended)

Best for trusted users like family or friends who will access the project regularly. They get full transparent access to both the web UI and Grafana iframes using normal LAN IPs — exactly the same experience as on your home network.

1. Go to [https://login.tailscale.com/admin/users](https://login.tailscale.com/admin/users)
2. Click **Invite users**
3. Enter their email address
4. They install Tailscale on their device and accept the invite
5. Share the web UI URL: `http://192.168.12.22/sump`
6. Grafana iframes load automatically since subnet routing handles `192.168.12.146:3000`

### Option B — Funnel URL (casual sharing)

Best for one-off sharing where you just want to send someone a link. They need no Tailscale install — just a browser. However Grafana iframes will **not** load for them since `192.168.12.146:3000` is unreachable without subnet routing.

Share the Funnel URL:
```
https://lucidpi.tailb986d2.ts.net
```

> **Note:** For full iframe support for non-tailnet users, Grafana would need to be exposed
> via a separate Funnel on port 3000 and the iframe URLs in `index3.h` updated accordingly.
> For most sharing scenarios, Option A (Tailscale invite) is simpler and more complete.

---

## Devices Currently on the Tailnet

| Device | Role |
|---|---|
| Raspberry Pi (`lucidpi`) | Subnet router, Grafana, InfluxDB |
| PC | Admin access |
| Phone | Mobile monitoring |

---

## Keeping Tailscale Updated

```bash
sudo tailscale update
```

---

## Troubleshooting

| Symptom | What to check |
|---|---|
| Can't reach `192.168.12.22` remotely | Confirm subnet route is approved in admin console |
| Grafana iframes blank remotely | Confirm device is on tailnet and subnet routing is active |
| Funnel URL not reachable | `tailscale funnel status` — if empty re-run `sudo tailscale funnel --bg 80` |
| Pi dropped off tailnet | `sudo tailscale up --advertise-routes=192.168.12.0/24` |
| `tailscale funnel` command not found | `sudo tailscale update` |
| Want to stop Funnel | `sudo tailscale funnel --bg=false 80` |
