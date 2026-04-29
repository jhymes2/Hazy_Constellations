// #include <Arduino.h>
// #include <SPI.h>
// #include <string.h>

// // PLS916H packet structure — 162 bytes total
// struct PLS916H_Packet {
//     uint8_t header[13];
//     uint8_t data[144];
//     uint8_t checksum;
//     uint8_t tail[4];
// } __attribute__((packed));

// static uint8_t frameData[144];  // LED bit map

// // Fixed header and tail from reverse engineered code
// static const uint8_t HEADER[13] = {
//     0x5A, 0xFF, 0x01, 0x5A, 0x24, 0x21,
//     0x3D, 0x01, 0x83, 0x5A, 0xFF, 0x02, 0x5B
// };
// static const uint8_t TAIL[4] = {0x5A, 0xFF, 0x04, 0x5D};

// // Checksum = simple sum of all 144 data bytes
// uint8_t calcChecksum(const uint8_t* data) {
//     uint8_t sum = 0;
//     for (int i = 0; i < 144; i++) sum += data[i];
//     return sum;
// }

// // Send full 162-byte SPI frame
// void sendFrame(const uint8_t* data) {
//     PLS916H_Packet pkt;
//     memcpy(pkt.header, HEADER, 13);
//     memcpy(pkt.data, data, 144);
//     pkt.checksum = calcChecksum(data);
//     memcpy(pkt.tail, TAIL, 4);

//     // 1MHz SPI, MSB first, Mode 0
//     SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
//     uint8_t* raw = (uint8_t*)&pkt;
//     for (size_t i = 0; i < sizeof(pkt); i++) SPI.transfer(raw[i]);
//     SPI.endTransaction();

//     // Flush pulse — required to latch frame
//     SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(0x00);
//     SPI.endTransaction();
// }

// // Set a specific bit in the frame buffer
// void setBit(int byteIndex, int bitIndex, bool on) {
//     if (byteIndex < 0 || byteIndex >= 144) return;
//     if (on) frameData[byteIndex] |=  (1 << bitIndex);
//     else    frameData[byteIndex] &= ~(1 << bitIndex);
// }

// // All LEDs on or off
// void allLEDs(bool on) {
//     memset(frameData, on ? 0xFF : 0x00, 144);
//     sendFrame(frameData);
// }

// // Halo LED positions from reverse engineered map
// const int haloBytes[] = {30,31,32,33,34,35,36,37,38,39,56,57,58,59};
// const int haloBits[]  = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
// const int HALO_COUNT  = 14;

// void setHalo(int idx, bool on) {
//     if (idx < 0 || idx >= HALO_COUNT) return;
//     setBit(haloBytes[idx], haloBits[idx], on);
// }

// void haloChase(int delayMs) {
//     for (int i = 0; i < HALO_COUNT; i++) {
//         setHalo(i, true);
//         sendFrame(frameData);
//         delay(delayMs);
//         setHalo(i, false);
//         sendFrame(frameData);
//     }
// }

// void setup() {
//     Serial.begin(115200);
//     SPI.begin();  // SCK=18, MOSI=23 on ESP32 DevKit V1

//     memset(frameData, 0, 144);

//     Serial.println("PLS916H SPI Display");

//     // Test 1 — all LEDs on
//     Serial.println("All ON");
//     allLEDs(true);
//     delay(1000);

//     // Test 2 — all LEDs off
//     Serial.println("All OFF");
//     allLEDs(false);
//     delay(500);

//     // Test 3 — halo chase
//     Serial.println("Halo chase");
//     haloChase(80);

//     // Test 4 — all on steady
//     allLEDs(true);
//     Serial.println("Done — all LEDs on");
// }

// void loop() {
//     // Refresh every 20ms to keep display alive
//     sendFrame(frameData);
//     delay(20);
// }