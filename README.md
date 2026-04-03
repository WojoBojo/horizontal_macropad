# WojRaf's macropad
A wireless 16 key horizontal macropad, made for Hack Club Blueprint 2026

I always liked the idea of macropads, but I didn't like the "box" style (usualy a 4 x 4 grid of buttons). What I wanted was a horizontal (1 x 16 grid of buttons) version, that I could put on top of my keyboard (like a row of function keys).

So I came up with something like this:
<img width="1799" height="977" alt="image" src="https://github.com/user-attachments/assets/1e150414-e076-45f2-b158-6a51324e17be" />

The biggest challenge was the PCB - I started learning how to make one in KiCad, but I ended up using EasyEDA because in my opinion it is a bit more user-friendly.
<img width="2282" height="287" alt="image" src="https://github.com/user-attachments/assets/96405e4d-842e-42b7-b497-d39067edd875" />
<img width="2285" height="281" alt="image" src="https://github.com/user-attachments/assets/bbfe332b-8061-45a3-906e-d197ee67a084" />

The hardware consists of:

- ESP32 S3 Wroom 1
- 16 Kalih BOX switches
- Alps Alpine rotary encoder
- USB-C port for charging and programing
- 18650 2600mAh 3.7V battery

# How to use
1. Plug in the macropad via cable to your computer.

2. Open the macropad.ino file in Arduino IDE, select ESP32S3 Dev Module as the board and flash the firmware.

2. With the macropad still connected via cable, open the serial monitor, and chose 115200 as the baud rate.

# Commands
list - shows the current key mapping.

bind - changes the key's function (example: bind 5 F1 - changes key nr. 5 to F1 (Function 1 key)).

save - saves the changes to flash (keeps data after restart)
