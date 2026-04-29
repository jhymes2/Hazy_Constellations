// //======================================== Help ========================================//
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


// // ======================================== Scan — blink every bit to map LEDs =======================================
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


// // ============================================= Halo helpers =============================================
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