# Horizontal Macropad 🎹

[![Hack Club Blueprint](https://img.shields.io/badge/Hack%20Club-Blueprint%202026-blueviolet)](https://hackclub.com)

A wireless 16-key horizontal macropad, designed for [Hack Club Blueprint 2026](https://blueprint.hackclub.com).

I always liked macropads but never liked the boxy 4×4 grid. I wanted a horizontal row — like a function key row — that sits on top of your keyboard. So I built one.

---

## ✨ Features

| Feature | Details |
|---------|---------|
| **16 keys** | Kailh BOX switches via CD74HC4067 mux (saves GPIOs) |
| **Rotary encoder** | EC11E1834403 (ALPS) — volume / scroll by default |
| **Battery** | 18650 2600mAh with BQ24074 charger + TPS63031 buck-boost |
| **USB-C** | Charging only (data lines unpopulated) |
| **Wireless** | ESP32-S3 + on-board PCB antenna |
| **Persistent keymap** | Save key bindings to flash — survive reboot |
| **Configurable** | Remap any key over serial — no reflashing needed |

---

## 📁 Repository Structure

```
hardware/            ← PCB design files
├── PCB.pdf          ← PCB layout printable
├── PCB.png          ← PCB layout preview
├── Schematic.pdf    ← Circuit schematic printable
├── Schematic.png    ← Circuit schematic preview
└── EasyEDA/         ← Editable source files (EasyEDA Pro)
    ├── PCB.epro
    └── Schematic.epro

case/                ← 3D-printed enclosure
├── MacropadCase.step   ← STEP file (CAD)
└── MacropadCase_.f3z   ← Fusion 360 archive

firmware/            ← ESP32-S3 code
├── HackClub_Macropad.ino       ← Original firmware (minimal)
└── HackClub_Macropad_new.ino   ← Full rewrite (encoder, battery, keymap)

manufacturing/       ← Ready-to-order files
└── gerbers.zip      ← PCB Gerber + drill + flying probe

BOM.csv              ← Bill of Materials (LCSC part numbers)

README.md            ← This file (original)
README_new.md        ← This file (updated)
```

---

## 🚀 Quick Start

### 1. Order the PCB
Upload `manufacturing/gerbers.zip` + `BOM.csv` to [JLCPCB](https://jlcpcb.com) or your preferred fab.

| Item | Est. cost (5 pcs) |
|------|-------------------|
| PCB 302×38mm (2-layer) | ~$20 |
| PCBA (top side assembly) | ~$40 |
| Components (LCSC) | ~$45 |
| **Total prototype** | **~$105** |

### 2. Print the case
`case/MacropadCase.step` — print on any FDM or resin printer.

### 3. Flash the firmware

**Recommended:** `firmware/HackClub_Macropad_new.ino`
- Open in **Arduino IDE 2.x**
- Board: `ESP32S3 Dev Module`
- Flash via **UART** (GPIO43=TX, GPIO44=RX) or solder 0Ω resistors on pads e40/e41 for native USB

> ⚠️ **Programming note:** USB-C on this board is charging-only. Data lines are unpopulated. Use a USB-UART adapter (e.g. CP2102) connected to GPIO43/44, or populate 0Ω resistors to enable native USB.

### 4. Configure key bindings

Open Serial Monitor at **115200 baud**:

```
list              → show current key map
bind 5 F1         → set key #5 to F1
bind 0 A          → set key #0 to letter A
save              → persist to flash (survives reboot)
load              → reload from flash
default           → restore factory defaults
battery           → show battery voltage
encoder           → show encoder position
```

### 5. Mount on keyboard
The horizontal form sits perfectly above your function keys. Use double-sided tape or screw holes (add your own — PCB doesn't include M3 holes in this rev).

---

## 🔌 Pin Map

| Signal | GPIO | Function |
|--------|------|----------|
| MUX_OUT | 1 | CD74HC4067 common (pull-up) |
| S0–S3 | 4–7 | Mux address lines |
| ENC_A | 18 | Rotary encoder phase A |
| ENC_B | 19 | Rotary encoder phase B (no conflict — USB data NC) |
| ENC_SW | — | **Not routed** on this PCB (pads D/E empty) |
| BAT_ADC | 39 | Battery voltage divider (ADC1) |

---

## 🛠 PCB Revision Notes

This is **v1.0** of the design. Things to fix in the next spin:

- [ ] **Add ground pour** — no copper fill on either layer → EMI risk for ESP32 @240MHz
- [ ] **Add M3 mounting holes** — 300mm board needs mechanical support
- [ ] **External pull-up on MUX_OUT** — internal INPUT_PULLUP (~45k) is marginal on long traces
- [ ] **Wire encoder push button** — pads D/E of EC11E1834403 are unconnected
- [ ] **TVS diode on VBUS** — ESD protection for USB-C
- [ ] **Thicker power traces** — VBAT and 3.3V should be ≥0.5mm wide

---

## 🏆 Made For Hack Club Blueprint 2026

Built from scratch — KiCad → EasyEDA → breadboard → PCB → case → firmware.

Thanks to [Hack Club](https://hackclub.com) for the opportunity and funding!