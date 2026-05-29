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
