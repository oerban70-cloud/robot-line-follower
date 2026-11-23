/*
 * Robot Line Follower — ESP32 + 2 Sensor Garis TCRT5000 + Driver L298N
 *
 * Robot mengikuti garis hitam di lantai putih secara otomatis:
 * dua sensor garis (kiri dan kanan) menjadi "mata" robot, ESP32 membaca
 * keduanya tiap 10 milidetik lalu mengatur kecepatan dua motor DC lewat
 * driver L298N dengan PWM.
 *
 * Logika gerak (sensor TCRT5000 mengeluarkan HIGH saat di atas garis
 * hitam, LOW saat di atas lantai putih — kalau modulmu terbalik, ubah
 * nilai SENSOR_GARIS_AKTIF di bawah menjadi LOW):
 *   kiri di garis, kanan di garis  -> maju lurus
 *   kiri di garis, kanan di luar   -> belok kiri
 *   kiri di luar,  kanan di garis  -> belok kanan
 *   kiri di luar,  kanan di luar   -> berhenti (garis hilang)
 *
 * Wiring (lihat schematic.png):
 *   Sensor kiri   OUT -> GPIO32, VCC -> 3V3, GND -> GND
 *   Sensor kanan  OUT -> GPIO33, VCC -> 3V3, GND -> GND
 *   L298N ENA     -> GPIO25 (PWM motor kiri)
 *   L298N IN1     -> GPIO26, IN2 -> GPIO27 (arah motor kiri)
 *   L298N IN3     -> GPIO18, IN4 -> GPIO19 (arah motor kanan)
 *   L298N ENB     -> GPIO13 (PWM motor kanan)
 *   L298N 12V     -> baterai 7,4 V (2x 18650), GND -> GND ESP32
 *   L298N 5V      -> VIN ESP32 (catu ESP32 dari regulator L298N)
 *   LED bawaan    -> GPIO2 (menyala saat robot mengikuti garis)
 *
 * Hanya memakai GPIO dan PWM bawaan ESP32 — tidak butuh library eksternal.
 */

#include <Arduino.h>

// ---------- Konfigurasi pin ----------
const uint8_t PIN_SENSOR_KIRI   = 32;  // OUT sensor garis kiri (TCRT5000)
const uint8_t PIN_SENSOR_KANAN  = 33;  // OUT sensor garis kanan (TCRT5000)
const uint8_t PIN_ENA           = 25;  // PWM motor kiri (L298N ENA)
const uint8_t PIN_IN1           = 26;  // Arah motor kiri
const uint8_t PIN_IN2           = 27;  // Arah motor kiri
const uint8_t PIN_IN3           = 18;  // Arah motor kanan
const uint8_t PIN_IN4           = 19;  // Arah motor kanan
const uint8_t PIN_ENB           = 13;  // PWM motor kanan (L298N ENB)
const uint8_t PIN_LED_STATUS    = 2;   // LED bawaan DevKit (GPIO2)

// ---------- Konstanta perilaku ----------
const bool    SENSOR_GARIS_AKTIF = HIGH;  // HIGH = sensor di atas garis hitam
const uint8_t KECEPATAN_LURUS    = 180;   // PWM 0-255 saat maju lurus
const uint8_t KECEPATAN_BELOK_LUAR  = 200;  // roda luar saat belok
const uint8_t KECEPATAN_BELOK_DALAM = 80;   // roda dalam saat belok
const uint8_t KECEPATAN_BERHENTI   = 0;     // roda berhenti
const uint32_t INTERVAL_BACA_MS   = 10;    // baca sensor tiap 10 ms
const uint32_t FREKUENSI_PWM_HZ   = 5000;  // frekuensi PWM motor
const uint8_t  RESOLUSI_PWM_BIT   = 8;     // resolusi PWM 8 bit (0-255)

// ---------- State ----------
uint32_t waktuBacaBerikutnya = 0;

// ---------- Kendali motor ----------
// Kecepatan bertanda: positif = maju, negatif = mundur, 0 = berhenti.
void aturMotorKiri(int kecepatan) {
  if (kecepatan >= 0) {
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    ledcWrite(PIN_ENA, (uint8_t)kecepatan);
  } else {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    ledcWrite(PIN_ENA, (uint8_t)(-kecepatan));
  }
}

void aturMotorKanan(int kecepatan) {
  if (kecepatan >= 0) {
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    ledcWrite(PIN_ENB, (uint8_t)kecepatan);
  } else {
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);
    ledcWrite(PIN_ENB, (uint8_t)(-kecepatan));
  }
}

void majuLurus() {
  aturMotorKiri(KECEPATAN_LURUS);
  aturMotorKanan(KECEPATAN_LURUS);
  digitalWrite(PIN_LED_STATUS, HIGH);
}

void belokKiri() {
  aturMotorKiri(KECEPATAN_BELOK_DALAM);
  aturMotorKanan(KECEPATAN_BELOK_LUAR);
  digitalWrite(PIN_LED_STATUS, HIGH);
}

void belokKanan() {
  aturMotorKiri(KECEPATAN_BELOK_LUAR);
  aturMotorKanan(KECEPATAN_BELOK_DALAM);
  digitalWrite(PIN_LED_STATUS, HIGH);
}

void berhenti() {
  aturMotorKiri(KECEPATAN_BERHENTI);
  aturMotorKanan(KECEPATAN_BERHENTI);
  digitalWrite(PIN_LED_STATUS, LOW);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  Serial.println("Robot Line Follower siap — letakkan di atas garis hitam.");

  pinMode(PIN_SENSOR_KIRI, INPUT);
  pinMode(PIN_SENSOR_KANAN, INPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  pinMode(PIN_LED_STATUS, OUTPUT);

  // PWM motor (API ledc core ESP32 3.x/4.x)
  ledcAttach(PIN_ENA, FREKUENSI_PWM_HZ, RESOLUSI_PWM_BIT);
  ledcAttach(PIN_ENB, FREKUENSI_PWM_HZ, RESOLUSI_PWM_BIT);

  berhenti();
}

// ---------- Loop utama ----------
void loop() {
  uint32_t sekarang = millis();
  if (sekarang < waktuBacaBerikutnya) {
    return;
  }
  waktuBacaBerikutnya = sekarang + INTERVAL_BACA_MS;

  bool kiriDiGaris  = (digitalRead(PIN_SENSOR_KIRI)  == SENSOR_GARIS_AKTIF);
  bool kananDiGaris = (digitalRead(PIN_SENSOR_KANAN) == SENSOR_GARIS_AKTIF);

  if (kiriDiGaris && kananDiGaris) {
    majuLurus();
  } else if (kiriDiGaris) {
    belokKiri();
  } else if (kananDiGaris) {
    belokKanan();
  } else {
    berhenti();
  }
}
