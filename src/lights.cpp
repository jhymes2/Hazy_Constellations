#include <Arduino.h>
#include <SPI.h>
#include <string.h>
#include <math.h>

// =============================================
// PLS916H LED Display — Idle Light Show
// ESP32 DevKit V1 — default VSPI bus
// SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23, SS=GPIO5
//
// Top row shows the hour, bottom row shows the minute —
// a real-time clock seeded from the build machine's clock
// at build time (via stamp_build_time.py, see platformio.ini)
// and kept running with millis(). No RTC chip or network
// sync: the clock resets to build time on every power-cycle/
// reboot and will drift slowly since nothing re-syncs it
// after boot.
//
// Halo blinks on 1s / off 1s.
// Every LED that isn't a number segment or halo LED
// (ambient/"other" bytes) fades in and out on its own
// independent, randomized timer — not synchronized.
// =============================================

struct PLS916H_Packet {
    uint8_t header[13];
    uint8_t data[144];
    uint8_t checksum;
    uint8_t tail[4];
} __attribute__((packed));

static uint8_t frame[144];

static const uint8_t HEADER[13] = {
    0x5A, 0xFF, 0x01, 0x5A, 0x24, 0x21,
    0x3D, 0x01, 0x83, 0x5A, 0xFF, 0x02, 0x5B
};
static const uint8_t TAIL[4] = {0x5A, 0xFF, 0x04, 0x5D};

static uint8_t calcChecksum(const uint8_t* data) {
    uint8_t sum = 0;
    for (int i = 0; i < 144; i++) sum += data[i];
    return sum;
}

static void write_pls916h(const uint8_t* data) {
    static PLS916H_Packet pkt;
    memcpy(pkt.header, HEADER, 13);
    memcpy(pkt.data, data, 144);
    pkt.checksum = calcChecksum(data);
    memcpy(pkt.tail, TAIL, 4);

    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    uint8_t* raw = (uint8_t*)&pkt;
    for (size_t i = 0; i < sizeof(pkt); i++) SPI.transfer(raw[i]);
    SPI.endTransaction();

    // Latch pulse
    SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE0));
    SPI.transfer(0x00);
    SPI.endTransaction();
}

static void setBit(int byteIndex, int bitIndex, bool on) {
    if (byteIndex < 0 || byteIndex >= 144) return;
    if (on) frame[byteIndex] |=  (1 << bitIndex);
    else    frame[byteIndex] &= ~(1 << bitIndex);
}

static void pushFrame() { write_pls916h(frame); }

// =============================================
// Digit display — 6 positions, order A,B,C,D,E,F,G.
// Digits 0/1 are partial (only light for "1", the hundreds
// column) — unused here since hour/minute never reach 100.
// =============================================
struct Segment { int byteIndex; int bitIndex; };

static Segment digitSegments[6][7] = {
    {{-1,-1},{7,3},{15,3},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
    {{-1,-1},{47,3},{55,3},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},
    {{0,3},{1,3},{2,3},{3,3},{4,3},{5,3},{6,3}},
    {{8,3},{9,3},{10,3},{11,3},{12,3},{13,3},{14,3}},
    {{40,3},{41,3},{42,3},{43,3},{44,3},{45,3},{46,3}},
    {{48,3},{49,3},{50,3},{51,3},{52,3},{53,3},{54,3}}
};

static const bool numberTable[10][7] = {
    {1,1,1,1,1,1,0}, // 0
    {0,1,1,0,0,0,0}, // 1
    {1,1,0,1,1,0,1}, // 2
    {1,1,1,1,0,0,1}, // 3
    {0,1,1,0,0,1,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {1,1,1,0,0,0,0}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

static void clearDigit(int d) {
    if (d < 0 || d > 5) return;
    for (int seg = 0; seg < 7; seg++) {
        Segment s = digitSegments[d][seg];
        if (s.byteIndex >= 0) setBit(s.byteIndex, s.bitIndex, false);
    }
}

static void displayDigit(int digitIndex, int value) {
    if (digitIndex < 0 || digitIndex > 5) return;
    clearDigit(digitIndex);
    if (value < 0 || value > 9) return;
    for (int seg = 0; seg < 7; seg++) {
        if (numberTable[value][seg]) {
            Segment s = digitSegments[digitIndex][seg];
            if (s.byteIndex >= 0) setBit(s.byteIndex, s.bitIndex, true);
        }
    }
}

// Display a row value 0-99 (hours/minutes never need the
// hundreds partial digit).
static void displayRow(int row, int value) {
    int idxTens = (row == 0) ? 2 : 4;
    int idxOnes = (row == 0) ? 3 : 5;
    if (value < 0) value = 0;
    if (value > 99) value = 99;
    displayDigit(idxTens, value / 10);
    displayDigit(idxOnes, value % 10);
}

// =============================================
// Real-time clock — seeded from the build machine's clock
// (__TIME__ is substituted by the compiler at build time).
// =============================================
static int bootHour, bootMinute, bootSecond;
static unsigned long bootMillis;
static int lastDisplayedHour = -1, lastDisplayedMinute = -1;

static void initClock() {
    // BUILD_HOUR/MINUTE/SECOND are injected fresh by stamp_build_time.py
    // on every build (unlike __TIME__, which only updates when this file
    // itself is recompiled — PlatformIO skips recompiling unchanged files,
    // so __TIME__ can silently go stale across uploads).
    bootHour   = BUILD_HOUR;
    bootMinute = BUILD_MINUTE;
    bootSecond = BUILD_SECOND;
    bootMillis = millis();
}

static void updateClock(unsigned long now) {
    unsigned long elapsedSec = (now - bootMillis) / 1000UL;
    unsigned long totalSec = (unsigned long)bootHour * 3600UL
                            + (unsigned long)bootMinute * 60UL
                            + (unsigned long)bootSecond
                            + elapsedSec;
    totalSec %= 86400UL; // wrap at 24h

    int hour   = totalSec / 3600UL;
    int minute = (totalSec % 3600UL) / 60UL;

    if (hour != lastDisplayedHour || minute != lastDisplayedMinute) {
        displayRow(0, hour);
        displayRow(1, minute);
        lastDisplayedHour = hour;
        lastDisplayedMinute = minute;
    }
}

// =============================================
// Byte groups
// =============================================

// Number-segment bytes (top row 0-15, bottom row 40-55) use bit 3 only.
static bool isNumberByte(int b) {
    return (b >= 0 && b <= 15) || (b >= 40 && b <= 55);
}

// Halo ring — bit 5 only.
static const int HALO_BYTES[] = {30,31,32,33,34,35,36,37,38,39,56,57,58,59};
static const int HALO_COUNT = 14;
static const int HALO_BIT = 5;

static bool isHaloByte(int b) {
    for (int i = 0; i < HALO_COUNT; i++) if (HALO_BYTES[i] == b) return true;
    return false;
}

// Bytes confirmed to stay dark — X mark (16-27), unmapped gaps
// (28,29,60,61,74,76), 112, and 143 — must not turn on with the fade.
static const int EXCLUDED_BYTES[] = {
    16,17,18,19,20,21,22,23,24,25,26,27,
    28,29,60,61,74,76,112,143,173
};
static const int EXCLUDED_COUNT = sizeof(EXCLUDED_BYTES) / sizeof(EXCLUDED_BYTES[0]);

static bool isExcludedByte(int b) {
    for (int i = 0; i < EXCLUDED_COUNT; i++) if (EXCLUDED_BYTES[i] == b) return true;
    return false;
}

// Everything else — ambient/"other" bytes only — each fades in and
// out on its own independent, randomized cycle.
static int fadeBytes[144];
static int fadeByteCount = 0;

struct FadeState { unsigned long cycleStart; unsigned long periodMs; };
static FadeState fadeStates[144];

static const unsigned long FADE_PERIOD_MIN_MS = 4000;
static const unsigned long FADE_PERIOD_MAX_MS = 8000;

static void buildFadeBytes() {
    fadeByteCount = 0;
    unsigned long now = millis();
    for (int b = 0; b < 144; b++) {
        if (isNumberByte(b) || isHaloByte(b) || isExcludedByte(b)) continue;
        fadeBytes[fadeByteCount] = b;
        unsigned long period = random(FADE_PERIOD_MIN_MS, FADE_PERIOD_MAX_MS);
        fadeStates[fadeByteCount].periodMs = period;
        // Random start offset so LEDs don't all begin in phase.
        fadeStates[fadeByteCount].cycleStart = now - random(0, (long)period);
        fadeByteCount++;
    }
}

// =============================================
// Animation state
// =============================================
static const unsigned long HALO_PERIOD_MS = 1000; // on 1s, off 1s
static bool haloOn = false;
static unsigned long haloTimer = 0;

static void updateHalo(unsigned long now) {
    if (now - haloTimer >= HALO_PERIOD_MS) {
        haloTimer = now;
        haloOn = !haloOn;
        for (int i = 0; i < HALO_COUNT; i++) setBit(HALO_BYTES[i], HALO_BIT, haloOn);
    }
}

// Exponent >1 biases the curve toward 0 — LEDs sit dark for most of
// the cycle and only briefly rise to a bright peak, instead of
// spending equal time on both sides of a plain sine wave.
static const float FADE_SHAPE_EXPONENT = 3.0f;

static void updateFade(unsigned long now) {
    for (int i = 0; i < fadeByteCount; i++) {
        FadeState &st = fadeStates[i];
        unsigned long elapsed = now - st.cycleStart;
        if (elapsed >= st.periodMs) {
            st.cycleStart = now;
            st.periodMs = random(FADE_PERIOD_MIN_MS, FADE_PERIOD_MAX_MS);
            elapsed = 0;
        }
        float phase = (float)elapsed / (float)st.periodMs; // 0..1
        float raw = sinf(phase * 2.0f * PI - HALF_PI) * 0.5f + 0.5f; // 0..1
        float shaped = powf(raw, FADE_SHAPE_EXPONENT);
        frame[fadeBytes[i]] = (uint8_t)(shaped * 255.0f);
    }
}

// =============================================
// Setup / Loop
// =============================================
void setup() {
    SPI.begin(); // default VSPI: SCK=18, MOSI=23, MISO=19, SS=5
    Serial.begin(9600);
    randomSeed(esp_random());
    memset(frame, 0, 144);
    buildFadeBytes();
    initClock();
    Serial.printf("Lights: clock started at %02d:%02d:%02d\n", bootHour, bootMinute, bootSecond);
}

void loop() {
    unsigned long now = millis();
    updateClock(now);
    updateHalo(now);
    updateFade(now);

    static unsigned long lastRefresh = 0;
    if (now - lastRefresh >= 20) {
        pushFrame();
        lastRefresh = now;
    }
}
