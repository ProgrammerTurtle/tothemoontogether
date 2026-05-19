# To The Moon Together

## Overview

<img width="4000" height="1849" alt="fulluptmtt1" src="https://github.com/user-attachments/assets/302ae31b-8613-4fb4-abe9-66de449a432f" />

To The Moon Together, or TTMT, is a 6 foot tall, 3” diameter, N-Class sub minimum diameter amateur sounding rocket - meaning a rocket carrying scientific payloads. N-Class being the motor class, and sub minimum diameter meaning that the motor casing is the main structure of the rocket - no additional body tube around it. This allows for very high performance - aiming for mach 3 and an altitude of 62 thousand feet (19KM).

Current Planned Launch: BALLS 34 in the Black Rock Desert, September 2026. 

## Payload

The payload features a three-board distributed instrumentation avionics stack using a CAN-FD bus — each board carries its own STM32G0B1CCU6 MCU, enabling redundant cross-validation and independent experiment processing

<img width="885" height="1822" alt="image" src="https://github.com/user-attachments/assets/a4e5c8b2-d334-44be-9102-22fa1764ef8c" />

**Experiment 1** is a Geiger counter: An SBM 21 geiger tube resides at the bottom of the payload for monitoring cosmis ray flux. At 62kft, the rocket should be nearing the Regener-Pfotzer maximum point of cosmic flux.

**Experiment 2** is atmospheric charge measurement: By referencing between the nose tip and rocket body, across the insulating fiberglass, we can hopefully monitor atmospheric gathered static charge during flight.

**Experiment 3** is an attempt to measure air density, and thus altitude, through structure resonance: By utilizing a piezo transducer embedded in the airframe, sine sweeps can be ran on the head end structure during descent. By monitoring resonance response, air density can hopefully be determined. From air density, altitude can be derived. 

To top it all off, there is a Runcam Split 4 v2 camera mounted in a custom shroud and heatsink unit to record footage of the entire flight. Provided the flight is completely nominal, footage of everything from launch to touchdown should be captured. 

<img width="4000" height="1109" alt="fullupttmt2" src="https://github.com/user-attachments/assets/5716c9e4-c4a1-4eb9-a136-97bd35d1bd1f" />


## This project was made possible by:
- Hackclub: Fallout
- Sunlu 
- And many friends within the amateur rocketry community
