/*
 * Horizontal Macropad - Complete Firmware
 * ESP32-S3, 16x Kailh BOX switches via CD74HC4067 mux
 * + EC11E1834403 rotary encoder (GPIO18=A, GPIO19=B, switch NC)
 * + Battery monitoring (ADC1 = GPIO4)
 *
 * USB HID Keyboard + persistent keymap in flash.
 *
 * PIN MAP:
 *   S0-S3   = GPIO4-7    → Mux address lines
 *   MUX_OUT = GPIO1      → Mux COM (INPUT_PULLUP)
 *   ENC_A   = GPIO18     → Encoder phase A
 *   ENC_B   = GPIO19     → Encoder phase B
 *   BAT_ADC = GPIO39     → ADC monitoring (divider to VBAT)
 *
 * NOTE: Encoder switch (push button) is NOT wired on this PCB revision.
 *       Code is included but #ifdef'd out.
 * NOTE: USB data lines are charging-only. Program via UART0 (GPIO43 TX, GPIO44 RX)
 *       or solder 0Ω on e40/e41 for native USB.
 */

#include <Arduino.h>
#include "USBHIDKeyboard.h"
#include <Preferences.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

// Mux
const uint8_t S0 = 4;
const uint8_t S1 = 5;
const uint8_t S2 = 6;
const uint8_t S3 = 7;
const uint8_t MUX_OUT = 1;

// Rotary encoder
const uint8_t ENC_A = 18;
const uint8_t ENC_B = 19;
// Encoder push button: DNP on this PCB (pads D/E not routed)
// #define ENC_SW  21   // uncomment + wire if you add it later

// Battery ADC
const uint8_t BAT_ADC = 39;  // GPIO39 = ADC1_CH4 (from schematic: Pad 39 -> ADC1)

// ============================================================
// KEY MAP — stored in RTC memory for fast access, synced to flash
// ============================================================

// Default key assignments (HID usage codes)
// 16 physical keys, in order SW6-SW9 (ch0-3), SW18-SW29 (ch4-15)
// We store HID usage IDs for generic desktop keyboard page.
static const uint8_t DEFAULT_KEYS[16] = {
  HID_KEY_F1,  HID_KEY_F2,  HID_KEY_F3,  HID_KEY_F4,       // SW6-9
  HID_KEY_F5,  HID_KEY_F6,  HID_KEY_F7,  HID_KEY_F8,       // SW18-21
  HID_KEY_F9,  HID_KEY_F10, HID_KEY_F11, HID_KEY_F12,      // SW22-25
  HID_KEY_F13, HID_KEY_F14, HID_KEY_F15, HID_KEY_F16,      // SW26-29
};

// For the serial "bind" command we accept these short names:
struct KeyName {
  const char* name;
  uint8_t hid;
};
static const KeyName KEY_NAMES[] = {
  {"F1",  HID_KEY_F1},   {"F2",  HID_KEY_F2},   {"F3",  HID_KEY_F3},
  {"F4",  HID_KEY_F4},   {"F5",  HID_KEY_F5},   {"F6",  HID_KEY_F6},
  {"F7",  HID_KEY_F7},   {"F8",  HID_KEY_F8},   {"F9",  HID_KEY_F9},
  {"F10", HID_KEY_F10},  {"F11", HID_KEY_F11},  {"F12", HID_KEY_F12},
  {"F13", HID_KEY_F13},  {"F14", HID_KEY_F14},  {"F15", HID_KEY_F15},
  {"F16", HID_KEY_F16},  {"F17", HID_KEY_F17},  {"F18", HID_KEY_F18},
  {"F19", HID_KEY_F19},  {"F20", HID_KEY_F20},  {"F21", HID_KEY_F21},
  {"F22", HID_KEY_F22},  {"F23", HID_KEY_F23},  {"F24", HID_KEY_F24},
  {"A",   HID_KEY_A},    {"B",   HID_KEY_B},    {"C",   HID_KEY_C},
  {"D",   HID_KEY_D},    {"E",   HID_KEY_E},    {"F",   HID_KEY_F},
  {"G",   HID_KEY_G},    {"H",   HID_KEY_H},    {"I",   HID_KEY_I},
  {"J",   HID_KEY_J},    {"K",   HID_KEY_K},    {"L",   HID_KEY_L},
  {"M",   HID_KEY_M},    {"N",   HID_KEY_N},    {"O",   HID_KEY_O},
  {"P",   HID_KEY_P},    {"Q",   HID_KEY_Q},    {"R",   HID_KEY_R},
  {"S",   HID_KEY_S},    {"T",   HID_KEY_T},    {"U",   HID_KEY_U},
  {"V",   HID_KEY_V},    {"W",   HID_KEY_W},    {"X",   HID_KEY_X},
  {"Y",   HID_KEY_Y},    {"Z",   HID_KEY_Z},
  {"0",   HID_KEY_0},    {"1",   HID_KEY_1},    {"2",   HID_KEY_2},
  {"3",   HID_KEY_3},    {"4",   HID_KEY_4},    {"5",   HID_KEY_5},
  {"6",   HID_KEY_6},    {"7",   HID_KEY_7},    {"8",   HID_KEY_8},
  {"9",   HID_KEY_9},
  {"ENTER",  HID_KEY_ENTER},  {"ESC",   HID_KEY_ESCAPE},
  {"TAB",    HID_KEY_TAB},    {"SPACE", HID_KEY_SPACE},
  {"BKSP",   HID_KEY_BACKSPACE}, {"DEL", HID_KEY_DELETE},
  {"INS",    HID_KEY_INSERT}, {"HOME",  HID_KEY_HOME},
  {"END",    HID_KEY_END},    {"PGUP",  HID_KEY_PAGE_UP},
  {"PGDN",   HID_KEY_PAGE_DOWN},
  {"LCTRL",  HID_KEY_CONTROL_LEFT},  {"RCTRL",  HID_KEY_CONTROL_RIGHT},
  {"LSHIFT", HID_KEY_SHIFT_LEFT},    {"RSHIFT", HID_KEY_SHIFT_RIGHT},
  {"LALT",   HID_KEY_ALT_LEFT},      {"RALT",   HID_KEY_ALT_RIGHT},
  {"LGUI",   HID_KEY_GUI_LEFT},      {"RGUI",   HID_KEY_GUI_RIGHT},
  {nullptr, 0}
};

RTC_DATA_ATTR uint8_t keyMap[16];
RTC_DATA_ATTR bool    keyMapLoaded = false;
Preferences prefs;

// ============================================================
// STATE
// ============================================================

USBHIDKeyboard Keyboard;

// Button debounce
bool   wasPressed[16] = {false};
uint32_t lastDebounce[16] = {0};
const uint32_t DEBOUNCE_US = 5000;  // 5ms

// Encoder
volatile int16_t encoderPos = 0;
int16_t          lastEncPos = 0;
uint32_t         lastEncTime = 0;

// Battery
uint32_t lastBatteryRead = 0;
const uint32_t BATTERY_INTERVAL_MS = 30000;  // every 30s
float batteryVoltage = 0.0f;

// ============================================================
// MUX HELPERS
// ============================================================

void setMux(int ch) {
  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);
  delayMicroseconds(10);  // plenty for CD74HC4067 settle (~200ns)
}

// ============================================================
// KEY LOOKUP
// ============================================================

uint8_t nameToHid(const char* name) {
  for (int i = 0; KEY_NAMES[i].name != nullptr; i++) {
    if (strcasecmp(name, KEY_NAMES[i].name) == 0)
      return KEY_NAMES[i].hid;
  }
  return 0;
}

const char* hidToName(uint8_t hid) {
  for (int i = 0; KEY_NAMES[i].name != nullptr; i++) {
    if (KEY_NAMES[i].hid == hid)
      return KEY_NAMES[i].name;
  }
  return "?";
}

// ============================================================
// ENCODER ISR
// ============================================================

void IRAM_ATTR encoderISR() {
  static uint8_t prev = 0;
  uint8_t ab = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  // Standard lookup table for KY-040 / EC11
  static const int8_t table[16] = {
    0,  1, -1,  0,   // 00→01=CW, 00→10=CCW
   -1,  0,  0,  1,   // 01→00=CCW, 01→11=CW
    1,  0,  0, -1,   // 10→00=CW, 10→11=CCW
    0, -1,  1,  0    // 11→01=CCW, 11→10=CW
  };
  int8_t step = table[(prev << 2) | ab];
  if (step) {
    encoderPos += step;
    prev = ab;
  }
}

// ============================================================
// SERIAL COMMANDS
// ============================================================

void printHelp() {
  Serial.println(F(
    "\nCommands:"
    "\n  list              - show current key mapping"
    "\n  bind <n> <key>    - set key n (0-15) to key name (F1, A, ENTER, etc.)"
    "\n  save              - save mapping to flash"
    "\n  load              - reload mapping from flash"
    "\n  default           - restore default mapping"
    "\n  battery           - show battery voltage"
    "\n  encoder           - show encoder position"
    "\n  resetenc          - reset encoder position to 0"
    "\n  help              - this message"
  ));
}

void cmdList() {
  Serial.println(F("\nCurrent keymap:"));
  for (int i = 0; i < 16; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "  %2d: %s (0x%02X)", i, hidToName(keyMap[i]), keyMap[i]);
    Serial.println(buf);
  }
  Serial.print(F("\nBattery: "));
  Serial.print(batteryVoltage, 2);
  Serial.println(F(" V"));
  Serial.print(F("Encoder: "));
  Serial.println(encoderPos);
}

void cmdBind(int n, const char* keyName) {
  if (n < 0 || n > 15) {
    Serial.println(F("Error: key index must be 0-15"));
    return;
  }
  uint8_t hid = nameToHid(keyName);
  if (hid == 0) {
    Serial.print(F("Error: unknown key '"));
    Serial.print(keyName);
    Serial.println(F("'. Type 'help' for names."));
    return;
  }
  keyMap[n] = hid;
  Serial.print(F("OK: key "));
  Serial.print(n);
  Serial.print(F(" -> "));
  Serial.println(hidToName(hid));
}

void cmdSave() {
  prefs.begin("macropad", false);
  prefs.putBytes("keymap", keyMap, 16);
  prefs.end();
  Serial.println(F("Saved to flash."));
}

void cmdLoad() {
  prefs.begin("macropad", true);
  size_t sz = prefs.getBytes("keymap", keyMap, 16);
  prefs.end();
  if (sz == 16) {
    Serial.println(F("Loaded from flash."));
  } else {
    Serial.println(F("No saved map, using defaults."));
    memcpy(keyMap, DEFAULT_KEYS, 16);
  }
}

void cmdDefault() {
  memcpy(keyMap, DEFAULT_KEYS, 16);
  Serial.println(F("Default keymap restored."));
}

void processSerial() {
  if (!Serial.available()) return;

  char line[64];
  size_t len = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
  line[len] = '\0';

  // Trim trailing whitespace
  while (len > 0 && (line[len-1] == '\r' || line[len-1] == ' ' || line[len-1] == '\t'))
    line[--len] = '\0';

  if (len == 0) return;

  char* cmd = strtok(line, " ");

  if (strcasecmp(cmd, "help") == 0) {
    printHelp();
  } else if (strcasecmp(cmd, "list") == 0) {
    cmdList();
  } else if (strcasecmp(cmd, "bind") == 0) {
    char* n_str = strtok(nullptr, " ");
    char* keyName = strtok(nullptr, " ");
    if (n_str && keyName) {
      cmdBind(atoi(n_str), keyName);
    } else {
      Serial.println(F("Usage: bind <0-15> <KEYNAME>"));
    }
  } else if (strcasecmp(cmd, "save") == 0) {
    cmdSave();
  } else if (strcasecmp(cmd, "load") == 0) {
    cmdLoad();
  } else if (strcasecmp(cmd, "default") == 0) {
    cmdDefault();
  } else if (strcasecmp(cmd, "battery") == 0) {
    readBattery();
    Serial.print(F("Battery: "));
    Serial.print(batteryVoltage, 2);
    Serial.println(F(" V"));
  } else if (strcasecmp(cmd, "encoder") == 0) {
    Serial.print(F("Encoder position: "));
    Serial.println(encoderPos);
  } else if (strcasecmp(cmd, "resetenc") == 0) {
    encoderPos = 0;
    Serial.println(F("Encoder reset to 0."));
  } else {
    Serial.print(F("Unknown: '"));
    Serial.print(cmd);
    Serial.println(F("'. Type 'help'."));
  }
}

// ============================================================
// BATTERY
// ============================================================

void readBattery() {
  // TODO: Calibrate divider ratio from actual schematic.
  // Default: assume 2:1 divider (max reading = 2*3.3V = 6.6V, OK for 4.2V Li-ion)
  int raw = analogRead(BAT_ADC);
  float mv = analogReadMilliVolts(BAT_ADC);  // ESP32-S3 has built-in millivolt read
  // If using voltage divider R3+R4: Vbatt = mV * (R3+R4)/R4
  // For 100k+100k divider: factor = 2.0
  // For now assume 2:1
  batteryVoltage = mv * 2.0f / 1000.0f;
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n=== Horizontal Macropad ==="));

  // Mux address pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(MUX_OUT, INPUT_PULLUP);

  // Encoder
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encoderISR, CHANGE);
  /*
  // If encoder switch is wired (not on this PCB rev):
  pinMode(ENC_SW, INPUT_PULLUP);
  */

  // Battery ADC
  analogReadResolution(12);  // 0-4095
  pinMode(BAT_ADC, INPUT);

  // Keymap
  if (!keyMapLoaded) {
    prefs.begin("macropad", true);
    size_t sz = prefs.getBytes("keymap", keyMap, 16);
    prefs.end();
    if (sz != 16) {
      memcpy(keyMap, DEFAULT_KEYS, 16);
    }
    keyMapLoaded = true;
  }

  Keyboard.begin();
  Serial.println(F("Keyboard ready."));

  // Initial battery read
  readBattery();
  Serial.print(F("Battery: "));
  Serial.print(batteryVoltage, 2);
  Serial.println(F(" V"));

  printHelp();
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  uint32_t now = micros();

  // --- 1. Scan key matrix ---
  for (int i = 0; i < 16; i++) {
    setMux(i);

    bool pressed = (digitalRead(MUX_OUT) == LOW);

    if (pressed && !wasPressed[i]) {
      if (now - lastDebounce[i] > DEBOUNCE_US) {
        lastDebounce[i] = now;
        wasPressed[i] = true;
        Keyboard.press(keyMap[i]);
      }
    } else if (!pressed && wasPressed[i]) {
      if (now - lastDebounce[i] > DEBOUNCE_US) {
        lastDebounce[i] = now;
        wasPressed[i] = false;
        Keyboard.release(keyMap[i]);
      }
    }
  }

  // --- 2. Encoder ---
  noInterrupts();
  int16_t pos = encoderPos;
  interrupts();

  if (pos != lastEncPos) {
    int16_t delta = pos - lastEncPos;
    lastEncPos = pos;
    // Send scroll/arrow keys based on direction
    while (delta > 0) {
      Keyboard.press(HID_KEY_ARROW_UP);
      delay(2);
      Keyboard.release(HID_KEY_ARROW_UP);
      delta--;
    }
    while (delta < 0) {
      Keyboard.press(HID_KEY_ARROW_DOWN);
      delay(2);
      Keyboard.release(HID_KEY_ARROW_DOWN);
      delta++;
    }
    lastEncTime = now;
  }

  /*
  // Encoder switch (uncomment when wired)
  static bool encSwLast = HIGH;
  bool encSwNow = digitalRead(ENC_SW);
  if (encSwLast == HIGH && encSwNow == LOW) {
    // Pressed: send ENTER or mute
    Keyboard.press(HID_KEY_ENTER);
    delay(2);
    Keyboard.release(HID_KEY_ENTER);
    delay(50);  // simple debounce
  }
  encSwLast = encSwNow;
  */

  // --- 3. Battery (periodic) ---
  if (millis() - lastBatteryRead > BATTERY_INTERVAL_MS) {
    readBattery();
    lastBatteryRead = millis();
  }

  // --- 4. Serial commands ---
  processSerial();

  delay(4);  // ~250 Hz loop
}