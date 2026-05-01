from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from influxdb import InfluxDBClient
from datetime import datetime, timezone
import logging
import os
import time

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="Sump Monitor Receiver")

VALID_EVENTS = {"flooding", "highwater", "allclear"}

# ── InfluxDB connection with retry ────────────────────────────────────────────
def get_influx_client(retries: int = 10, delay: int = 3) -> InfluxDBClient:
    host     = os.getenv("INFLUXDB_HOST", "influxdb")
    port     = int(os.getenv("INFLUXDB_PORT", 8086))
    username = os.getenv("INFLUXDB_USER", "sump")
    password = os.getenv("INFLUXDB_PASSWORD", "changeme123")
    database = os.getenv("INFLUXDB_DB", "sumpmonitor")

    for attempt in range(1, retries + 1):
        try:
            c = InfluxDBClient(
                host=host,
                port=port,
                username=username,
                password=password,
                database=database
            )
            c.ping()
            logger.info("InfluxDB connection established.")
            return c
        except Exception as e:
            logger.warning(f"InfluxDB not ready (attempt {attempt}/{retries}): {e}")
            time.sleep(delay)

    raise RuntimeError("Could not connect to InfluxDB after multiple attempts.")

client = get_influx_client()

# ── Models ────────────────────────────────────────────────────────────────────
class SumpEvent(BaseModel):
    type:        str
    location:    str   = "sump_pit"
    distance:    float = 0.0
    pitActivity: float = 0.0
    dtStamp:     str   = ""

# ── Routes ────────────────────────────────────────────────────────────────────
@app.post("/event")
def receive_event(event: SumpEvent):
    if event.type not in VALID_EVENTS:
        raise HTTPException(
            status_code=400,
            detail=f"Invalid type. Must be one of {VALID_EVENTS}"
        )

    payload = [{
        "measurement": "sump_events",
        "time": datetime.now(timezone.utc).isoformat(),
        "tags": {
            "event_type": event.type,
            "location":   event.location
        },
        "fields": {
            "value":       1,
            "distance":    event.distance,
            "pitActivity": event.pitActivity,
            "dtStamp":     event.dtStamp
        }
    }]

    client.write_points(payload)
    logger.info(
        f"Event logged: {event.type} | "
        f"distance: {event.distance} | "
        f"pitActivity: {event.pitActivity}"
    )

    return {
        "status":      "ok",
        "event":       event.type,
        "distance":    event.distance,
        "pitActivity": event.pitActivity
    }


@app.get("/health")
def health():
    try:
        client.ping()
        db_status = "ok"
    except Exception:
        db_status = "unreachable"
    return {"status": "ok", "influxdb": db_status}


@app.get("/recent")
def recent_events(limit: int = 20):
    limit = max(1, min(limit, 100))  # clamp between 1 and 100
    result = client.query(
        f"SELECT * FROM sump_events ORDER BY time DESC LIMIT {limit}"
    )
    return {"events": list(result.get_points())}
