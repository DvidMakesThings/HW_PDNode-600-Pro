# PDNode 600 Pro  
## High-Performance USB PD Power Distribution Unit – 10" Rackmount

![Hardware Development Status](https://img.shields.io/badge/status-Ongoing-orange)

## Description  
[ENERGIS](https://github.com/DvidMakesThings/HW_10-In-Rack_PDU) is the 230V managed PDU project. PDNode-600 Pro is the modular DC/USB-C sibling for setups
where mains switching is not desired, but controllable, rack-friendly power output is still needed.

The PDNode-600 is a professional-grade, 600W USB Power Delivery Unit designed specifically for compact 10-inch rack systems, targeting embedded developers, homelab enthusiasts, test benches, and smart infrastructure. It delivers regulated, high-current power across 8x independent USB-C PD outputs and 4x fixed 5V USB-A outputs.

At the core of the PDNode platform is a centralized industrial-grade 24V/25A power supply, distributing regulated DC to dedicated per-port buck or buck-boost converters. Each USB-C port is managed by an MPQ4242-AEC1 PD controller, enabling clean, autonomous negotiation of power profiles up to 20V @ 3A or 5V @ 5A with eMarker cable support. All power conversion, protection, and monitoring are designed to meet high reliability and safety standards.

The PDNode-600 Pro, by contrast, is built for remote-managed racks and headless environments, featuring Ethernet connectivity, a clean web-based UI, SNMP integration, and firmware updates via USB-C. 

---

## Specifications 

### Input
- **Input voltage:** 230 VAC, max 4A 
- **System topology:** 24V distribution on the baseboard, per-slot conversion on PD cards

### Output (per PD card / per slot)
- **8x independent USB-C PD source**:
  - 5V 3A
  - 9V 3A
  - 12V 3A
  - 15V 3A
  - 20V 3A
  - 5V 5A (only with 5A e-marked cable)
  - 20V 5A (only with 5A e-marked cable)
- **4x USB-A source** up to **5V / 1A** per port (depending on configuration and limits)

---

## Features
- **8x independent USB-C PD source ports** (modular PD cards, one port per slot)
- **Per-port PD profiles**
- **Total system capability: up to 600W** (shared input power budget)
- **4x USB-A auxiliary outputs** (5V, 1A per port) for low-power devices
- **Ethernet connectivity** for control / monitoring
- **USB service interface** (debug, configuration, firmware update, or log access depending on implementation)
- **Per-port power monitoring** (current/voltage telemetry via onboard sensing)
- **Per-port status reporting** (PGOOD / fault indication, and software-readable status)
- **Hot-swappable slot concept** (cards can be replaced without redesigning the baseboard)

---

## Hardware architecture

### Baseboard
- 24V DC input from industrial power supply
- Power distribution to all slots (high-current bus)
- Slot connectors + slot management (card detect, enable, status aggregation)
- I2C fanout per slot (mux) for monitoring and controlling devices
- Ethernet interface for user interaction
- USB/service interface for debug/config/updates
- 4x USB-A outputs
- 8x USB-C PD outputs

### [BladeCore-M54E](https://github.com/DvidMakesThings/HW_BladeCore-M54E)
- RP2354B microcontroller
- External QSPI flash memory
- External I²C EEPROM
- 100BASE Ethernet via W5500
- Dual‑path USB connectivity with automatic source selection
- Onboard power regulation and defined power‑up sequencing
- Compact M.2 M-key form factor

### PD Card
- 1x USB-C PD source port (one card = one port)
- Local power conversion to generate VBUS from the 24V bus
- PD controller + USB-C port control signals (enable, reset, IRQ)
- Per-port current/voltage measurement
- Per-port power good / fault status output
- Optional ID EEPROM footprint (DNI) for traceability/calibration

---

## Schematics
The full schematics are available:
- **[Baseboard Schematics](src/PDF/SCH_BladeCore-M54_100.pdf)** 
- **[PDCard-4242 Schematics](src/PDF/SCH_PDCard-4242_100.pdf)**
- **[BladeCore-M54E Schematics](src/PDF/SCH_PDCard-4242_100.pdf)**

---

## Development Phases (Hardware + Firmware)

| Hardware Phase                          | Status          | Firmware Phase                         | Status         |
| --------------------------------------- | --------------- | -------------------------------------- | -------------- |
| Architecture definition                 | ✅ Completed    | Slot scan concept (TCA9548A)           | 🔵 Planned     |
| Baseboard PCB Design                    | ✅ Completed    | I2C driver + mux control               | 🔵 Planned     |
| PD Card PCB Design                      | ✅ Completed    | INA219 monitoring                      | 🔵 Planned     |
| Documentation                           | 🟠 Ongoing      | PD controller interface (SPI + GPIO)   | 🔵 Planned     |
| PCB ordering                            | 🟠 Ongoing      | Telemetry + status reporting           | 🔵 Planned     |
| Prototyping and Hardware Bring-up       | 🟠 Ongoing      | Bring-up scripts / debug tooling       | 🔵 Planned     |
| Validation (load, thermal, long-run)    | 🔵 Planned      | Error handling + fault reporting       | 🔵 Planned     |
| Production optimization                 | 🔵 Planned      | Integration and polishing              | 🔵 Planned     |

---

## Changelog 
#### Rev0.1.0:
- Initial design
- STUSB4710 controller + TPS552882

#### Rev1.0.0:
- Full redesign. STUSB4710 became EOL, need to use different PD controller
- New concept: modular Baseboard + PDCard architecture
- PDCard power stage (MPQ4242-AEC1)
- Baseboard: Per-slot I2C muxing (TCA9548A)
- Optional per-card EEPROM (not used, but capable)

<table>
  <tr>
    <td align="center">
      <img src="images/1.0.0/PDNode-PDCard-4242_TOP.png" alt="PDCard 3D Top View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>PDCard Top</b>
    </td>
    <td align="center">
      <img src="images/1.0.0/PDNode-PDCard-4242_BOTTOM.png" alt="PDCard 3D Bottom View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>PDCard Bottom</b>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="images/1.0.0/BladeCore-M54E_Top.png" alt="PDCard 3D Top View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>BladeCore-M54E Top</b>
    </td>
    <td align="center">
      <img src="images/1.0.0/BladeCore-M54E_Bottom.png" alt="PDCard 3D Bottom View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>BladeCore-M54E Bottom</b>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="images/1.0.0/PDNode_Baseboard-Top.png" alt="BaseBoard 3D Top View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>BladeCore-M54E Top</b>
    </td>
    <td align="center">
      <img src="images/1.0.0/PDNode_Baseboard-Bottom.png" alt="BaseBoard 3D Bottom View" style="width: 100%; min-width: 200px; border-radius: 6px;">
      <br><b>BladeCore-M54E Bottom</b>
    </td>
  </tr>
</table>

---

## License
### Software Components
This project's software is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.
See the [Software License](LICENSE-AGPL) file for details.

#### What AGPL-3.0 means:

- ✅ **You can** freely use, modify, and distribute this software
- ✅ **You can** use this project for personal, educational, or internal purposes
- ✅ **You can** contribute improvements back to this project

- ⚠️ **You must** share any modifications you make if you distribute the software
- ⚠️ **You must** release the source code if you run a modified version on a server that others interact with
- ⚠️ **You must** keep all copyright notices intact

- ❌ **You cannot** incorporate this code into proprietary software without sharing your source code
- ❌ **You cannot** use this project in a commercial product without either complying with AGPL or obtaining a different license

### Hardware Components
Hardware designs, schematics, and related documentation are licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0 International License)**. See the [Hardware License](LICENSE-CC-BY-NC-SA) file for details.

#### What CC BY-NC-SA 4.0 means:

- ✅ **You can** study, modify, and distribute the hardware designs
- ✅ **You can** create derivative works for personal, educational, or non-commercial use
- ✅ **You can** build this project for your own personal use

- ⚠️ **You must** give appropriate credit and indicate if changes were made
- ⚠️ **You must** share any modifications under the same license terms
- ⚠️ **You must** include the original license and copyright notices

- ❌ **You cannot** use the designs for commercial purposes without explicit permission
- ❌ **You cannot** manufacture and sell products based on these designs without a commercial license
- ❌ **You cannot** create closed-source derivatives for commercial purposes
- ❌ **You cannot** use the designer's trademarks without permission

### Commercial & Enterprise Use

Commercial use of this project is prohibited without obtaining a separate commercial license. If you are interested in:

- Manufacturing and selling products based on these designs
- Incorporating these designs into commercial products
- Any other commercial applications

Please contact me through any of the channels listed in the [Contact](#contact) section to discuss commercial licensing arrangements. Commercial licenses are available with reasonable terms to support ongoing development.

## Contact

For questions or feedback:
- **Email:** [dvidmakesthings@gmail.com](mailto:dvidmakesthings@gmail.com)
- **GitHub:** [DvidMakesThings](https://github.com/DvidMakesThings)

## Contributing

Contributions are welcome! As this is an early-stage project, please reach out before 
making substantial changes:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/concept`)
3. Commit your changes (`git commit -m 'Add concept'`)
4. Push to the branch (`git push origin feature/concept`)
5. Open a Pull Request with a detailed description
