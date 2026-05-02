# Sump-Monitor Mounting Use Case

## Overview

This document describes a recommended physical mounting solution for the Sump-Monitor sensor and project box, developed for a real-world deployment. The design uses standard PVC fittings throughout — no corrosion, no special tools, fully serviceable.

---

## Hardware Context

- **Sensor:** Ultrasonic distance sensor with 75° beam angle, 2.6m cable, JST connector
- **Project Box (PB):** Sealed enclosure housing ESP32-S3 (N16R8) and breadboard
- **Calming Well:** 2" PVC pipe extending from pit interior to above floor level
- **Pit:** Open top sump pit, away from walls, 9'+ ceiling — no wall or ceiling anchor practical

---

## Design Goals

- Stable, permanent mount with no wall or ceiling attachment required
- Sensor centered and unobstructed for accurate distance readings
- Project box accessible for lid removal and maintenance
- Sensor cable (2.6m max) managed cleanly
- All PVC construction — corrosion proof for basement/pit environment
- Serviceable without tools for pump access

---

## Assembly Description

### 1. Below Floor — Pit Anchor Framework

Using 3/4" or 1" PVC tube and tee couplings, construct two cross arms spanning the pit diameter at floor level. Tee couplings at each end of the cross arms orient downward, with short vertical tab sections contacting the pit liner wall. This anchors the entire structure inside the pit without fasteners.

The cross arms box around the 2" calming well PVC, holding it centered and plumb.

- Standard PVC tee fittings — hardware store
- Cut cross arm lengths to pit diameter
- Solvent weld once positioned correctly
- Stainless screws into metal pit liner optional for additional security

### 2. Calming Well Riser — 2" PVC

2" PVC pipe passes through the cross arm framework and extends:

- **Down** into the pit — calms water surface turbulence for accurate sensor readings
- **Up** through floor level, extending 1 to 2 feet above floor

Top of riser terminated with a **2" PVC end cap**. Sensor mounted centered in end cap looking straight down through the open riser. At above-floor height the 75° beam angle is unobstructed — no pipe wall interference.

### 3. Project Box Vertical Mount — 3/4" PVC

A single **Tee fitting oriented vertically upward** is installed on one of the floor-level cross arms. A 3/4" PVC vertical pipe rises from this Tee alongside the 2" calming well riser.

The project box mounts to this 3/4" vertical pipe via a **3/4" PVC flange epoxied to the bottom of the project box**.

- Flange bonded to exterior bottom of project box with marine epoxy or JB Weld
- Rough up mating surfaces with sandpaper; clean with isopropyl before bonding
- Epoxy-only bond (no bolts through bottom) avoids conflict with breadboard mounted on interior floor of project box
- Project box slides onto 3/4" pipe — height adjustable before final positioning
- Top lid of project box remains fully accessible for maintenance

---

## Sensor Installation

### End Cap Preparation

1. Drill **18mm hole centered** in 2" PVC end cap

### Sensor Seating and Cable Routing

2. From the **water facing side** (underside) of end cap, feed sensor cable up through 18mm hole to ceiling facing side (top)
3. Sensor body follows cable up through hole from below
4. Sensor **flange seats flush against water facing side** (underside) of end cap — natural stop, sensor faces water
5. Apply **RTV silicone** around sensor body on ceiling facing side (top) — seals and locks sensor in place
6. Allow RTV to cure before deployment

### Cable Routing to Project Box

7. Sensor cable exits ceiling facing side of end cap
8. Routes down alongside 3/4" PB mount pipe — cable tie or clips as needed
9. **JST connector feeds through waterproof gland** on project box — gland sized for JST connector body
10. RTV or silicone applied around cable at gland to seal gap between cable OD and gland ID
11. JST connector connects to sensor PCB inside project box

---

## Parts List

| Item | Spec | Notes |
|------|------|-------|
| Calming well pipe | 2" PVC | Length: pit depth + 1-2 ft above floor |
| Calming well end cap | 2" PVC | 18mm hole drilled centered for sensor |
| Cross arm pipe | 3/4" or 1" PVC | Cut to pit diameter x2 |
| Tee fittings | 3/4" or 1" PVC | Cross arm ends (tabs) + 1 upward for PB mount |
| Project box vertical | 3/4" PVC | Height to suit project box position |
| PVC flange | 3/4" | Epoxied to project box bottom |
| Waterproof gland | Sized for JST connector body | Mounted in project box wall |
| PVC solvent cement | Standard | All joints |
| Marine epoxy or JB Weld | — | Flange to project box bond |
| RTV silicone sealant | — | Sensor to end cap seal; gland cable gap |
| Isopropyl alcohol | — | Surface prep before epoxy and RTV |

---

## Installation Notes

- Dry fit all components before solvent welding — verify calming well is plumb
- Sensor cable slack taken up along the 3/4" PB mount pipe — cable tie or clips as needed
- Gland sized for JST connector body — RTV fills gap around smaller cable OD once JST is pulled through
- Flange position on project box bottom: verify no conflict with breadboard layout before bonding
- RTV on sensor end cap and gland: allow full cure time before deployment

---

## Sensor Beam Angle

The ultrasonic sensor has a 75° beam angle. Mounting inside 2" PVC would cause beam reflections off pipe walls. This design avoids that by:

- Terminating the 2" riser with an end cap **above floor level**
- Sensor faces **open air** looking down into the open riser
- Calming well effect is preserved below floor in the pit
- Accurate, unobstructed readings at all water levels

---

## Serviceability

- Pump access: cross arm framework sized to pit opening — full pit access maintained around the riser
- Project box: top lid screws fully accessible — flange mount on bottom does not interfere
- Sensor: RTV seal cuttable for sensor removal if service required
- Entire above-floor assembly can be lifted out if cross arm tabs are not permanently fastened

---

*This mounting solution was developed for the Tech500/Sump-Monitor project. See main README for software installation and configuration.*
