// #include <Arduino.h>
// #include <SPI.h>
// #include <string.h>

// // =============================================
// // PLS916H LED Display Controller
// // ESP32 DevKit V1 — default VSPI bus
// // SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23, SS=GPIO5
// // =============================================

// // Status/button pins — kept off the SPI bus (18/19/23/5) and UART0 (1/3)
// #define STATUS_PIN 25
// #define BTN1_PIN   26
// #define BTN2_PIN   27

// struct PLS916H_Packet {
//     uint8_t header[13];
//     uint8_t data[144];
//     uint8_t checksum;
//     uint8_t tail[4];
// } __attribute__((packed));

// static uint8_t frame[144];

// // Fixed protocol header and tail
// static const uint8_t HEADER[13] = {
//     0x5A, 0xFF, 0x01, 0x5A, 0x24, 0x21,
//     0x3D, 0x01, 0x83, 0x5A, 0xFF, 0x02, 0x5B
// };
// static const uint8_t TAIL[4] = {0x5A, 0xFF, 0x04, 0x5D};

// // =============================================
// // Segment mapping — 6 digit positions
// // Order: A, B, C, D, E, F, G
// // Digits 0-1 are partial (only B,C = segments for "1")
// // =============================================
// struct Segment { int byteIndex; int bitIndex; };

// static Segment digitSegments[6][7] = {
//     // Digit 0 — partial top (hundreds top row)
//     {{-1,-1},{7,3},{15,3},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
//     // Digit 1 — partial bottom (hundreds bottom row)
//     {{-1,-1},{47,3},{55,3},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
//     // Digit 2 — full top tens
//     {{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3}},
//     // Digit 3 — full top ones
//     {{8,3},{9,3},{10,3},{11,3},{12,3},{13,3},{14,3}},
//     // Digit 4 — full bottom tens
//     {{40,3},{41,3},{42,3},{43,3},{44,3},{45,3},{46,3}},
//     // Digit 5 — full bottom ones
//     {{48,3},{49,3},{50,3},{51,3},{52,3},{53,3},{54,3}}
// };

// // Halo LED positions
// static const Segment haloMap[] = {
//     {30,5},{31,5},{32,5},{33,5},{34,5},{35,5},
//     {36,5},{37,5},{38,5},{39,5},{56,5},{57,5},{58,5},{59,5}
// };
// static const int HALO_COUNT = 14;

// // 7-segment truth table 0-9 (A B C D E F G)
// static const bool numberTable[10][7] = {
//     {1,1,1,1,1,1,0}, // 0
//     {0,1,1,0,0,0,0}, // 1
//     {1,1,0,1,1,0,1}, // 2
//     {1,1,1,1,0,0,1}, // 3
//     {0,1,1,0,0,1,1}, // 4
//     {1,0,1,1,0,1,1}, // 5
//     {1,0,1,1,1,1,1}, // 6
//     {1,1,1,0,0,0,0}, // 7
//     {1,1,1,1,1,1,1}, // 8
//     {1,1,1,1,0,1,1}  // 9
// };

// // =============================================
// // Low-level SPI send
// // =============================================
// static uint8_t calcChecksum(const uint8_t* data) {
//     uint8_t sum = 0;
//     for (int i = 0; i < 144; i++) sum += data[i];
//     return sum;
// }

// void write_pls916h(const uint8_t* data) {
//     static PLS916H_Packet pkt;
//     memcpy(pkt.header, HEADER, 13);
//     memcpy(pkt.data, data, 144);
//     pkt.checksum = calcChecksum(data);
//     memcpy(pkt.tail, TAIL, 4);

//     SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
//     uint8_t* raw = (uint8_t*)&pkt;
//     for (size_t i = 0; i < sizeof(pkt); i++) SPI.transfer(raw[i]);
//     SPI.endTransaction();

//     // Latch pulse
//     SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(0x00);
//     SPI.endTransaction();
// }

// // =============================================
// // Bit helpers
// // =============================================
// void setBit(int byteIndex, int bitIndex, bool on) {
//     if (byteIndex < 0 || byteIndex >= 144) return;
//     if (on) frame[byteIndex] |=  (1 << bitIndex);
//     else    frame[byteIndex] &= ~(1 << bitIndex);
// }

// void clearAll()    { memset(frame, 0x00, 144); }
// void allOn()       { memset(frame, 0xFF, 144); }
// void pushFrame()   { write_pls916h(frame); }

// // =============================================
// // Digit display
// // =============================================
// void clearDigit(int d) {
//     if (d < 0 || d > 5) return;
//     for (int seg = 0; seg < 7; seg++) {
//         Segment s = digitSegments[d][seg];
//         if (s.byteIndex >= 0) setBit(s.byteIndex, s.bitIndex, false);
//     }
// }

// void displayDigit(int digitIndex, int value) {
//     if (digitIndex < 0 || digitIndex > 5) return;
//     clearDigit(digitIndex);

//     // Partial digits (0,1) only support value=1
//     if (digitIndex <= 1) {
//         if (value != 1) return;
//         for (int seg = 0; seg < 7; seg++) {
//             Segment s = digitSegments[digitIndex][seg];
//             if (s.byteIndex >= 0) setBit(s.byteIndex, s.bitIndex, true);
//         }
//         return;
//     }

//     if (value < 0 || value > 9) return;
//     for (int seg = 0; seg < 7; seg++) {
//         if (numberTable[value][seg]) {
//             Segment s = digitSegments[digitIndex][seg];
//             if (s.byteIndex >= 0) setBit(s.byteIndex, s.bitIndex, true);
//         }
//     }
// }

// // Display a row value 0-999
// void displayRow(int row, int value) {
//     int idx_partial = (row == 0) ? 0 : 1;
//     int idx_tens    = (row == 0) ? 2 : 4;
//     int idx_ones    = (row == 0) ? 3 : 5;

//     clearDigit(idx_partial);
//     clearDigit(idx_tens);
//     clearDigit(idx_ones);

//     if (value < 0) return;

//     int hundreds = value / 100;
//     int tens     = (value / 10) % 10;
//     int ones     = value % 10;

//     if (hundreds == 1) displayDigit(idx_partial, 1);
//     if (value >= 10)   displayDigit(idx_tens, tens);
//     displayDigit(idx_ones, ones);
// }

// // Display number 0-999999 split across two rows
// void displayNumber(long num) {
//     if (num < 0) num = 0;
//     if (num > 999999) num = 999999;

//     clearAll();
//     if (num <= 999) {
//         displayRow(0, (int)num);
//         displayRow(1, -1);
//     } else {
//         displayRow(0, (int)(num / 1000L));
//         displayRow(1, (int)(num % 1000L));
//     }
//     pushFrame();
// }

// // =============================================
// // Halo helpers
// // =============================================
// void setHalo(int idx, bool on) {
//     if (idx < 0 || idx >= HALO_COUNT) return;
//     setBit(haloMap[idx].byteIndex, haloMap[idx].bitIndex, on);
// }

// void haloAll(bool on) {
//     for (int i = 0; i < HALO_COUNT; i++) setHalo(i, on);
// }

// void haloChase(int delayMs = 60) {
//     for (int i = 0; i < HALO_COUNT; i++) {
//         setHalo(i, true);  pushFrame(); delay(delayMs);
//         setHalo(i, false); pushFrame();
//     }
// }

// void haloBlink(int times, int ms = 150) {
//     for (int i = 0; i < times; i++) {
//         haloAll(true);  pushFrame(); delay(ms);
//         haloAll(false); pushFrame(); delay(ms);
//     }
// }

// // =============================================
// // Help
// // =============================================
// void printHelp() {
//     Serial.println(F("\n===== PLS916H DISPLAY CONTROLLER ====="));
//     Serial.println(F("NUMBER COMMANDS:"));
//     Serial.println(F("  num <N>              Show number 0-999999"));
//     Serial.println(F("  top <N>              Show 0-999 on top row only"));
//     Serial.println(F("  bot <N>              Show 0-999 on bottom row only"));
//     Serial.println(F("  inc                  Increment current number by 1"));
//     Serial.println(F("  dec                  Decrement current number by 1"));
//     Serial.println(F("  inc <N>              Increment by N"));
//     Serial.println(F("  dec <N>              Decrement by N"));
//     Serial.println(F(""));
//     Serial.println(F("DIGIT COMMANDS:"));
//     Serial.println(F("  digit <pos> <val>    Set digit at pos 1-6 to val 0-9"));
//     Serial.println(F("  digits <a> <b> <c> <d> <e> <f>   Set all 6 digits"));
//     Serial.println(F(""));
//     Serial.println(F("HALO COMMANDS:"));
//     Serial.println(F("  halo 1|0             Halo all on/off"));
//     Serial.println(F("  hchase [ms]          Halo chase animation"));
//     Serial.println(F("  hblink [n] [ms]      Blink halo N times"));
//     Serial.println(F("  hset <idx> 1|0       Set individual halo LED 0-13"));
//     Serial.println(F(""));
//     Serial.println(F("RAW COMMANDS:"));
//     Serial.println(F("  set <byte> <bit>     Turn ON bit"));
//     Serial.println(F("  clr <byte> <bit>     Turn OFF bit"));
//     Serial.println(F("  all 1|0              All segments on/off"));
//     Serial.println(F("  clear                Clear display"));
//     Serial.println(F("  scan                 Blink every bit (find LEDs)"));
//     Serial.println(F("  help                 Show this menu"));
//     Serial.println(F("=======================================\n"));
// }

// // =============================================
// // Scan — blink every bit to map LEDs
// // =============================================
// void scanAll() {
//     Serial.println(F("SCAN: blinking every bit. Watch display and note."));
//     uint8_t backup[144]; memcpy(backup, frame, 144);
//     clearAll(); pushFrame();

//     for (int b = 0; b < 144; b++) {
//         for (int bit = 0; bit < 8; bit++) {
//             Serial.print(F("byte=")); Serial.print(b);
//             Serial.print(F(" bit=")); Serial.println(bit);

//             setBit(b, bit, true);  pushFrame(); delay(300);
//             setBit(b, bit, false); pushFrame(); delay(100);
//         }
//     }
//     memcpy(frame, backup, 144);
//     pushFrame();
//     Serial.println(F("SCAN done."));
// }

// // =============================================
// // State
// // =============================================
// static long currentNumber = 0;

// // =============================================
// // Setup / Loop — disabled while src/lights.cpp is
// // the active sketch (only one setup()/loop() pair
// // can be linked at a time). Uncomment to switch back.
// // =============================================
// // void setup() {
// //     pinMode(STATUS_PIN, OUTPUT);
// //     pinMode(BTN1_PIN, INPUT_PULLDOWN);
// //     pinMode(BTN2_PIN, INPUT_PULLDOWN);
// //
// //     SPI.begin(); // default VSPI: SCK=18, MOSI=23, MISO=19, SS=5
// //     Serial.begin(9600);
// //     memset(frame, 0, 144);
// //
// //     // Startup animation
// //     haloChase(50);
// //     haloAll(true); pushFrame(); delay(200);
// //     haloAll(false);
// //
// //     displayNumber(0);
// //     Serial.println(F("PLS916H ready."));
// //     printHelp();
// // }
// //
// // void loop() {
// //     digitalWrite(STATUS_PIN, (digitalRead(BTN1_PIN) || digitalRead(BTN2_PIN)) ? HIGH : LOW);
// //
// //     // Refresh every 20ms
// //     static unsigned long lastRefresh = 0;
// //     if (millis() - lastRefresh >= 20) {
// //         pushFrame();
// //         lastRefresh = millis();
// //     }
// //
// //     if (!Serial.available()) return;
// //
// //     String cmd = Serial.readStringUntil('\n');
// //     cmd.trim();
// //     if (cmd.length() == 0) return;
// //
// //     // --- num ---
// //     if (cmd.startsWith("num")) {
// //         long val = 0;
// //         if (sscanf(cmd.c_str(), "num %ld", &val) == 1) {
// //             currentNumber = constrain(val, 0L, 999999L);
// //             displayNumber(currentNumber);
// //             Serial.print(F("num = ")); Serial.println(currentNumber);
// //         } else Serial.println(F("ERR: num <0-999999>"));
// //     }
// //
// //     // --- top ---
// //     else if (cmd.startsWith("top")) {
// //         int val = 0;
// //         if (sscanf(cmd.c_str(), "top %d", &val) == 1) {
// //             displayRow(0, constrain(val, 0, 999));
// //             pushFrame();
// //             Serial.print(F("top = ")); Serial.println(val);
// //         } else Serial.println(F("ERR: top <0-999>"));
// //     }
// //
// //     // --- bot ---
// //     else if (cmd.startsWith("bot")) {
// //         int val = 0;
// //         if (sscanf(cmd.c_str(), "bot %d", &val) == 1) {
// //             displayRow(1, constrain(val, 0, 999));
// //             pushFrame();
// //             Serial.print(F("bot = ")); Serial.println(val);
// //         } else Serial.println(F("ERR: bot <0-999>"));
// //     }
// //
// //     // --- inc ---
// //     else if (cmd.startsWith("inc")) {
// //         long step = 1;
// //         sscanf(cmd.c_str(), "inc %ld", &step);
// //         currentNumber = constrain(currentNumber + step, 0L, 999999L);
// //         displayNumber(currentNumber);
// //         Serial.print(F("num = ")); Serial.println(currentNumber);
// //     }
// //
// //     // --- dec ---
// //     else if (cmd.startsWith("dec")) {
// //         long step = 1;
// //         sscanf(cmd.c_str(), "dec %ld", &step);
// //         currentNumber = constrain(currentNumber - step, 0L, 999999L);
// //         displayNumber(currentNumber);
// //         Serial.print(F("num = ")); Serial.println(currentNumber);
// //     }
// //
// //     // --- digit pos val ---
// //     else if (cmd.startsWith("digit ")) {
// //         int pos, val;
// //         if (sscanf(cmd.c_str(), "digit %d %d", &pos, &val) == 2
// //             && pos >= 1 && pos <= 6 && val >= 0 && val <= 9) {
// //             displayDigit(pos - 1, val);
// //             pushFrame();
// //             Serial.println(F("OK"));
// //         } else Serial.println(F("ERR: digit <1-6> <0-9>"));
// //     }
// //
// //     // --- digits a b c d e f ---
// //     else if (cmd.startsWith("digits")) {
// //         int a,b,c,d,e,f;
// //         if (sscanf(cmd.c_str(), "digits %d %d %d %d %d %d",
// //                    &a,&b,&c,&d,&e,&f) == 6) {
// //             clearAll();
// //             int v[6] = {a,b,c,d,e,f};
// //             for (int i = 0; i < 6; i++)
// //                 if (v[i] >= 0 && v[i] <= 9) displayDigit(i, v[i]);
// //             pushFrame();
// //             Serial.println(F("OK"));
// //         } else Serial.println(F("ERR: digits <d1> <d2> <d3> <d4> <d5> <d6>"));
// //     }
// //
// //     // --- halo ---
// //     else if (cmd.startsWith("halo")) {
// //         int on;
// //         if (sscanf(cmd.c_str(), "halo %d", &on) == 1) {
// //             haloAll(on != 0);
// //             pushFrame();
// //             Serial.println(on ? F("Halo ON") : F("Halo OFF"));
// //         } else Serial.println(F("ERR: halo 1|0"));
// //     }
// //
// //     // --- hchase ---
// //     else if (cmd.startsWith("hchase")) {
// //         int ms = 60;
// //         sscanf(cmd.c_str(), "hchase %d", &ms);
// //         haloChase(ms);
// //         Serial.println(F("OK"));
// //     }
// //
// //     // --- hblink ---
// //     else if (cmd.startsWith("hblink")) {
// //         int n = 3, ms = 150;
// //         sscanf(cmd.c_str(), "hblink %d %d", &n, &ms);
// //         haloBlink(n, ms);
// //         Serial.println(F("OK"));
// //     }
// //
// //     // --- hset idx on ---
// //     else if (cmd.startsWith("hset")) {
// //         int idx, on;
// //         if (sscanf(cmd.c_str(), "hset %d %d", &idx, &on) == 2) {
// //             setHalo(idx, on != 0);
// //             pushFrame();
// //             Serial.println(F("OK"));
// //         } else Serial.println(F("ERR: hset <0-13> <1|0>"));
// //     }
// //
// //     // --- set byte bit ---
// //     else if (cmd.startsWith("set")) {
// //         int x, y;
// //         if (sscanf(cmd.c_str(), "set %d %d", &x, &y) == 2) {
// //             setBit(x, y, true); pushFrame();
// //             Serial.println(F("OK"));
// //         } else Serial.println(F("ERR: set <byte> <bit>"));
// //     }
// //
// //     // --- clr byte bit ---
// //     else if (cmd.startsWith("clr")) {
// //         int x, y;
// //         if (sscanf(cmd.c_str(), "clr %d %d", &x, &y) == 2) {
// //             setBit(x, y, false); pushFrame();
// //             Serial.println(F("OK"));
// //         } else Serial.println(F("ERR: clr <byte> <bit>"));
// //     }
// //
// //     // --- all 1|0 ---
// //     else if (cmd.startsWith("all")) {
// //         int on;
// //         if (sscanf(cmd.c_str(), "all %d", &on) == 1) {
// //             if (on) allOn(); else clearAll();
// //             pushFrame();
// //             Serial.println(on ? F("All ON") : F("All OFF"));
// //         } else Serial.println(F("ERR: all 1|0"));
// //     }
// //
// //     // --- clear ---
// //     else if (cmd == "clear") {
// //         clearAll(); currentNumber = 0; pushFrame();
// //         Serial.println(F("Cleared"));
// //     }
// //
// //     // --- scan ---
// //     else if (cmd == "scan") {
// //         scanAll();
// //     }
// //
// //     // --- help ---
// //     else if (cmd == "help") {
// //         printHelp();
// //     }
// //
// //     else {
// //         Serial.print(F("Unknown: ")); Serial.println(cmd);
// //         Serial.println(F("Type 'help' for commands."));
// //     }
// // }


