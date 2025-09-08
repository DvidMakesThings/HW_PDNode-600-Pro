# PDNode 600 / PDNode 600 Pro  
## High-Performance USB PD Power Distribution Unit – 10" Rackmount


## Description  
The PDNode 600 is a professional-grade, 600W USB Power Delivery Unit designed specifically for compact 10-inch rack systems, targeting embedded developers, homelab enthusiasts, test benches, and smart infrastructure. It delivers precision-regulated, high-current power across 8x independent USB-C PD outputs and 4x fixed 5V USB-A outputs, from a standard 230V AC input.

At the core of the PDNode platform is a centralized industrial-grade 24V/25A power supply, distributing regulated DC to dedicated per-port buck or buck-boost converters. Each USB-C port is managed by an STUSB4710 PD controller, enabling clean, autonomous negotiation of power profiles up to 20V @ 3A or 5V @ 5A with eMarker cable support. All power conversion, protection, and monitoring are designed to meet high reliability and safety standards.

The PDNode 600 features local control via physical buttons, LED indicators, and an onboard OLED display.
The PDNode 600 Pro, by contrast, is built for remote-managed racks and headless environments, featuring Ethernet connectivity, a clean web-based UI, SNMP integration, and firmware updates via USB-C. It omits all physical buttons and indicators in favor of streamlined software-based interaction.

Both models share the same core electrical platform and mechanical design, ensuring a consistent user experience and upgrade path.

---

## Technical Specifications  

### Power Input  
- **Connector**: IEC C13 inlet with fuse  
- **Voltage**: 230V AC nominal  
- **Frequency**: 50/60 Hz  
- **Protection**:  
    - Internal fuse  
    - MOV surge suppression  
    - Common-mode EMI choke  
- **Conversion**:  
    - Mean Well RSP-600-24  
    - 24V DC @ 25A continuous output  
    - Efficiency: Up to 90%  
    - Built-in fan and overload protection  

---

### USB-C Power Delivery Outputs (8x)  
- **Connector**: USB-C 2.0 receptacles  
- **Controller**: STUSB4710 (1 per port)  
- **Negotiated PD Profiles**:  
    - 5V @ up to 5A (with eMarker cable)  
    - 9V / 12V / 15V / 20V @ up to 3A (per port)  
- **Power Regulation**:  
    - Dedicated buck or buck converter per port  
    - Programmable PDO selection via resistor or I²C  
- **Protection**:  
    - Overvoltage, undervoltage, overcurrent  
    - Thermal shutdown on converter stage  
- **Features**:  
    - Per-port enable/disable control  
    - Per-port current and voltage monitoring  

---

### USB-A Outputs (4x)  
- **Connector**: USB-A 2.0 receptacles  
- **Output**: Fixed 5V @ 1A per port  
- **Regulation**: Independent DC-DC converters with current limit  
- **Protection**:  
    - Overcurrent  

---

### Microcontroller  
- **Model**: RP2040 (dual-core ARM Cortex-M0+ @ 133 MHz)  
- **Role**:  
    - Monitoring  
    - Controls power switches  
    - Handles user input (600 only)  
    - Serves web UI and SNMP interface (600 Pro only)  
    - Manages display and status feedback  

---

## Control and Monitoring  

### PDNode 600 (Base Model)  
- **Physical Control**:  
    - 3x Button (Left, Right, Toggle)  
    - 1x Fault LED per port (status feedback)  
- **Display**:  
    - 0.96" OLED screen (monochrome)  
    - Displays port voltage, load current, port state  
- **Network Features**: None  

### PDNode 600 Pro (Managed Model)  
- **Physical Control**: None  
- **Ethernet Interface**: 10/100 Mbps  
- **Web UI**:  
    - Per-port status (voltage/current)  
    - Enable/disable control   
- **SNMP Support**: Custom SNMP for port monitoring and control  
- **Firmware Update**: Via USB-C device mode (mass storage or DFU)  

---

## PCB and Electrical Design  
- **PCB**: 4-layer  
    - Layer 1: Power distribution (1oz)  
    - Layer 2: Ground plane (0.5oz)  
    - Layer 3: Control signal routing (0.5oz)  
    - Layer 4: Power distribution (1oz)  
- **Current Handling**:  
    - 2x 13mm power pours (top & bottom) with stitched vias  
    - Supports up to 25A sustained  
- **Thermal Design**:  
    - Optimized for active or passive airflow  
    - Heat spread via copper pours and converter thermal pads  

---

## Mechanical  
- **Form Factor**: 10-inch rack-mountable, 1U height  
- **Enclosure**: Powder-coated aluminum chassis 
- **Front-Facing Control Interface, Back-Facing Ports**  
- **Mounting**: Front flanges with M4 screw holes  
- **Rear ventilation cutouts**

---

## Power Budget  
- **Total Output Capacity**: 600W  
    - 8x USB-C ports @ max 60W = 480W  
    - 4x USB-A ports @ 5W = 20W  
    - 100W reserved headroom for conversion losses and efficiency  
- **Input Current**: ~2.4–2.6A @ 230V AC (full load)  

---

## Applications  
- Raspberry Pi clusters (up to 8-node)  
- Homelab power infrastructure  
- Remote embedded hardware labs  
- USB-PD powered NAS, SSD, and networking gear  
- Automated test setups  
- Data center micro-racks and portable rigs  

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
Hardware designs, schematics, and related documentation are licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0)**. See the [Software License](LICENSE-CC BY-NC-SA) file for details.

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