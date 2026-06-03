# To The Moon Together

## Overview

<img width="4000" height="1849" alt="fulluptmtt1" src="https://github.com/user-attachments/assets/302ae31b-8613-4fb4-abe9-66de449a432f" />

To The Moon Together, or TTMT, is a 6 foot tall, 3” diameter, N-Class sub minimum diameter amateur sounding rocket - meaning a rocket carrying scientific payloads. N-Class being the motor class, and sub minimum diameter meaning that the motor casing is the main structure of the rocket - no additional body tube around it. This allows for very high performance - aiming for mach 3 and an altitude of 62 thousand feet (19KM).

Current Planned Launch: BALLS 34 in the Black Rock Desert, September 2026. 

## Flight Avionics

TTMT will be flying a MetrumITX and Featherweight Blue Jay inside of the nose cone avionics bay. These two computers are responsible for parachute deployment and flight logging. 

## Payload

The payload features a three-board distributed instrumentation avionics stack using a CAN-FD bus — each board carries its own STM32G0B1CCU6 MCU, enabling redundant cross-validation and independent experiment processing

<img width="885" height="1822" alt="image" src="https://github.com/user-attachments/assets/a4e5c8b2-d334-44be-9102-22fa1764ef8c" />

**Experiment 1** is a Geiger counter: An SBM 21 geiger tube resides at the bottom of the payload for monitoring cosmis ray flux. At 62kft, the rocket should be nearing the Regener-Pfotzer maximum point of cosmic flux.

**Experiment 2** is atmospheric charge measurement: By referencing between the nose tip and rocket body, across the insulating fiberglass, atmospheric gathered static charge can theoretically be monitored during flight.

**Experiment 3** is an attempt to measure air density, and thus altitude, through structure resonance: By utilizing a piezo transducer embedded in the airframe, sine sweeps can be ran on the head end structure during descent. By monitoring resonance response, air density can hopefully be determined. From air density, altitude can be derived. 

To top it all off, there is a Runcam Split 4 v2 camera mounted in a custom shroud and heatsink unit to record footage of the entire flight. Provided the flight is completely nominal, footage of everything from launch to touchdown should be captured. 

That's right, a total of 4 fully custom PCBs. 

<img width="4000" height="1109" alt="fullupttmt2" src="https://github.com/user-attachments/assets/5716c9e4-c4a1-4eb9-a136-97bd35d1bd1f" />

---

BOM is visible here: https://docs.google.com/spreadsheets/d/1I6cnVDX1UCcmCTm6yOHvnUr30Lxsm3f6hqrUW3u3K9U/edit?usp=sharing
Or attached as a .CSV

## Why am I building this?

For a long time, I have had a passion for rocketry. I started off doing model rockets with my dad and it eventually grew into an obsession with high power rocketry. Whether it's developing machines for producing rocket parts (like a carbon fiber tube winder that I am using for my fin can tube) or actual rockets, I have spent hundreds, if not thousands of hours working on this hobby.
I got an opportunity to do a really high performance build. I was making a camera module for a 3" rocket for a friend and realized - hey, why don't I make a rocket for this camera module too? After all, I have to get a minimum of 2 pcbs assembled per design.
This is that rocket. I have poured so many hours into this, I have almost failed classes over this, I have lost sleep over this. Making it into a scientific platform allows me to merge my passions in rocketry, physics, math, and general science in a way I haven't been able to before.
So. Godspeed!

## PCB Photos and Wiring Diagrams

<details>
  <summary>Click to Expand</summary>

## PCB Photos

### Camera PCB
<img width="972" height="1026" alt="image" src="https://github.com/user-attachments/assets/9d37d268-56aa-4323-8874-b924455a63ca" />
<img width="1193" height="845" alt="image" src="https://github.com/user-attachments/assets/4d7ff867-38f1-4340-8af1-584551ca58f1" />

---

### Instrumentation MCU PCB
<img width="1801" height="1146" alt="image" src="https://github.com/user-attachments/assets/56f8a75b-bfc1-4b84-820d-9edc817a8928" />
<img width="1193" height="845" alt="image" src="https://github.com/user-attachments/assets/e5acbf63-9e8c-47b4-8ec9-5ea7e930ffad" />

---

### Instrumentation Sensor PCB
<img width="1829" height="1152" alt="image" src="https://github.com/user-attachments/assets/488fe1e1-af67-4d1f-9cd5-180fd22283b2" />
<img width="1203" height="845" alt="image" src="https://github.com/user-attachments/assets/bcb7344a-6b0d-4afb-88b4-9d75e00ab206" />

---

### Instrumentation Geiger PCB
<img width="1828" height="1130" alt="image" src="https://github.com/user-attachments/assets/c730b8a6-d4f6-414d-bdc4-9e6ebcd9fce6" />
<img width="1190" height="845" alt="image" src="https://github.com/user-attachments/assets/2cb70bfa-4861-4e11-b04e-b4031bea31f4" />

---

### Instrumentation Flex PCB Cables
<img width="1367" height="160" alt="image" src="https://github.com/user-attachments/assets/9938a771-9f42-45de-a66c-62b7bc32da57" />
<img width="1344" height="134" alt="image" src="https://github.com/user-attachments/assets/a48ecd59-7cf8-40ee-950e-94aec5624500" />
<img width="1190" height="845" alt="image" src="https://github.com/user-attachments/assets/629dd3bf-5004-400d-8e64-69fc2f338454" />

---

## Wiring Diagrams

### Nosecone AV Bay Wiring
<img width="1965" height="1206" alt="image" src="https://github.com/user-attachments/assets/e0df4aad-167d-439e-8e3a-6f23b1061339" />

---

### Payload Wiring
<img width="1922" height="1196" alt="image" src="https://github.com/user-attachments/assets/e10b8dd7-01b4-40a1-b762-8b4cbe4e9593" />



</details>

## This project was made possible by:
- Hackclub: Fallout
- Sunlu 
- And many friends within the amateur rocketry community


## BOM

| Item                                            | Quantity     | Cost      | Source                                                                                      | Purpose                                                                                                            |                                                             |
|-------------------------------------------------|--------------|-----------|---------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------|
| STRUCTURE                                       |              |           |                                                                                             |                                                                                                                    |                                                             |
| "3"" Light Fiberglass Sleeve                    | 20ft"        | 1         | $20.00                                                                                      | https://sollercompositesllc.com/product/natural-uncolored-3in-light-fiberglass-sleeves/                            | Nose cone and chute cannon layups                           |
| Aeropoxy Quart Kit                              | 1            | $110.00   | https://www.aircraftspruce.com/catalog/pnpages/01-42135.php?                                | Nose Cone, Chute Cannon, and Fin Can layups                                                                        |                                                             |
| ELECTRONICS                                     |              |           |                                                                                             |                                                                                                                    |                                                             |
| STLink USB Debugger                             | 1            | $3.00     | https://www.aliexpress.us/item/3256807639661526.html?                                       | Debugging and Programming Boards                                                                                   |                                                             |
| Wio Tracker L1 Pro                              | 1            | $48.00    | https://www.seeedstudio.com/Wio-Tracker-L1-Pro-p-6454.html                                  | Base Station for Camera Board - Non-custom for Reliability Purposes                                                |                                                             |
| LSM6DSO32TR Accelerometer                       | 1            | $12.00    | https://www.aliexpress.us/item/3256811581419858.html?                                       | Accelerometer - Resonant Response Experiment                                                                       |                                                             |
| MetrumITX Board + Ground Station                | 1            | $240.00   | Kev1n8088 on Slack                                                                          | Tracking, Flight Control                                                                                           |                                                             |
| Elvin Beacon                                    | 1            | N/A       | Already Owned                                                                               | Tracking                                                                                                           |                                                             |
| Reperix Tracker                                 | 1            | N/A       | Already Owned                                                                               | Tracking                                                                                                           |                                                             |
| Runcam Split 4 V2 Camera                        | 1            | $90.00    | https://www.aliexpress.us/item/3256808492130639.html?                                       | Recording the awesome flight                                                                                       |                                                             |
| 1s 300mah Lipo Battery                          | 14           | $22.00    | https://www.aliexpress.us/item/3256811737992102.html?                                       | Powering everything - one 1s4p pack + one 1s3p pack + 4 standalone cells + 4 redundant cells                       |                                                             |
| RECOVERY                                        |              |           |                                                                                             |                                                                                                                    |                                                             |
| Ematches, Firewire MJG                          | 20           | $20.00    | https://www.csrocketry.com/recovery-supplies/ejection-supplies/firewire-electric-match.html | Recovery Charge Firing                                                                                             |                                                             |
| Custom Drogue and Main Parachute                | 2            | $140.00   | Parachute Friend                                                                            | Recovering the rocket - landing                                                                                    |                                                             |
| 3/8"" Flat Kevlar                               | 1            | N/A       | Already Owned                                                                               | Attaching Parachutes to Rocket - Shock Cord                                                                        |                                                             |
| MOTOR                                           |              |           |                                                                                             |                                                                                                                    |                                                             |
| "3"" Aluminum Tube                              | 6 Feet"      | 1         | $145.00                                                                                     | https://www.dxengineering.com/parts/dxe-at1317                                                                     | Motor Casing Stock, will be cut and drilled to size by Me.  |
| Nozzle Assembly - Aluminum, Phenolic, Graphite  | 1            | $230.00   | Local Machinist                                                                             | For Motor Nozzle                                                                                                   |                                                             |
| HARDWARE                                        |              |           |                                                                                             |                                                                                                                    |                                                             |
| McMaster-Carr Shipping and Tax                  | 1            | $30.00    | https://www.mcmaster.com                                                                    | Shipping and Tax                                                                                                   |                                                             |
| 6-32 1/2"" Countersunk Torx Bolt                | 10           | $13.00    | https://www.mcmaster.com/90920A148/                                                         | Payload Mounting                                                                                                   |                                                             |
| 1/4-20 Eye Hanger Bolt                          | 1            | $18.00    | https://www.mcmaster.com/90172A542/                                                         | Recovery Anchor                                                                                                    |                                                             |
| M2 Countersunk Torx Screws, M5                  | 20           | $8.00     | https://www.mcmaster.com/90236A103/                                                         | Camera Mounting                                                                                                    |                                                             |
| 2-56 Low Profile Nut                            | 10           | $5.00     | https://www.mcmaster.com/90730A003/                                                         | Payload Assembly                                                                                                   |                                                             |
| 2-56 Threaded Rod, 1ft                          | 1            | $4.00     | https://www.mcmaster.com/90034A460/                                                         | Payload Assembly                                                                                                   |                                                             |
| 3mm Dowel Pins, 8mm Long                        | 10           | $10.00    | https://www.mcmaster.com/93600A303/                                                         | Chute Cannon Attachment                                                                                            |                                                             |
| "3-48 Socket Head Screws                        | 1/2"" Long"  | 10        | $6.00                                                                                       | https://www.mcmaster.com/92185A098/                                                                                | Internal Coupler Assembly                                   |
| "3-48 Socket Head Screws                        | 1/4"" Long"  | 10        | $5.00                                                                                       | https://www.mcmaster.com/92185A096/                                                                                | Internal Coupler Assembly                                   |
| "2-56 Button Head Hex Screws                    | 5/16"" Long" | 20        | $7.00                                                                                       | https://www.mcmaster.com/92949A078/                                                                                | Payload Assemby                                             |
| 1/4' Diameter 5/16"" Long Steel Dowel Pins      | 20           | $22.00    | https://www.mcmaster.com/98381A535/                                                         | Forward Motor Assembly                                                                                             |                                                             |
| 1/4-20 Set Screws, Flat Tip,3/8                 | 25           | $7.50     | https://www.mcmaster.com/94355A535/                                                         | Aft Motor Assembly                                                                                                 |                                                             |
| "6-32 Button Head Hex Screw                     | 5/16"""      | 16        | $5.00                                                                                       | https://www.mcmaster.com/92949A145/                                                                                | Radax Assemblies                                            |
| MISCELLANEOUS                                   |              |           |                                                                                             |                                                                                                                    |                                                             |
| Purple Mica Pigment                             | 1            | $14.00    | https://a.co/d/01c2ZzY7                                                                     | Coloring Nonmetallic Parts/Accents                                                                                 |                                                             |
| Partall Mold Release Wax                        | 1            | $17.00    | https://a.co/d/0fLlNBUW                                                                     | Mold Release for Fiberglass/CF part molds.                                                                         |                                                             |
| 915mhz LORA Antenna                             | 4            | $6.00     | https://www.aliexpress.us/item/3256805050972049.html?                                       | Payload Board Antennas                                                                                             |                                                             |
| 500C Thermistor `                               | 1            | $10.00    | https://www.aliexpress.us/item/2255800405055939.html?                                       | Nose Tip Temperature Measurements                                                                                  |                                                             |
| PCBWay Non-Sponsored Cost                       | 1            | $25.00    | pcbway.com                                                                                  | PCBWay only offered a fixed amount.                                                                                |                                                             |
| JLCCNC Misc. Cost for Custom PCBS And CNC Parts | 1            | $460.00   | jlccnc.com, fedex.com                                                                       | OSHWLAB Stars only covers 80% of project cost, so there is a small cost applicable. Shipping through FedEx aswell. |                                                             |
| 20mm Piezo Transducer                           | 1            | $2.00     | https://www.aliexpress.us/item/3256812072131171.html?                                       | Payload Experiment - Resonant Response                                                                             |                                                             |
| SBM-21 Geiger Tube                              | 1            | $36.00    | https://www.ebay.com/itm/388693213334?                                                      | Payload Experiment - Radiation                                                                                     |                                                             |
| MMCX RG316 Coax Cable, 10CM                     | 1            | $3.00     | https://a.aliexpress.com/_mt1c4Fz                                                           | Payload Experiment - Atmospheric Charge                                                                            |                                                             |
| MMCX Female to SMA cable , 30cm                 | 1            | $3.00     | https://a.aliexpress.com/_mL6sZkL                                                           | Payload Experiment - Atmospheric Charge                                                                            |                                                             |
| MMCX-KWE Female Right Angle Board Connector     | 1            | $3.00     | https://www.aliexpress.us/item/3256807429972438.html?                                       | Payload Experiment - Atmospheric Charge                                                                            |                                                             |
|                                                 |              |           |                                                                                             |                                                                                                                    |                                                             |
| GRAND TOTAL                                     |              | $1,799.50 |                                                                                             |                                                                                                                    |                                                             |
