// Robot Line Follower — ESP32 + 2 sensor garis TCRT5000 + driver L298N
//
// Robot mengikuti garis hitam di lantai putih tanpa disentuh.
// Dua sensor garis TCRT5000 di depan membaca garis; ESP32 memutuskan
// arah dari kombinasi kedua sensor, lalu mengatur kecepatan kedua motor
// DC lewat driver L298N dengan sinyal PWM.
//
// Logika mengikuti garis (sensor HIGH = di atas garis hitam):
//   kiri HIGH, kanan HIGH  -> maju lurus
//   kiri HIGH saja         -> belok kiri (motor kiri pelan)
//   kanan HIGH saja        -> belok kanan (motor kanan pelan)
//   keduanya LOW           -> putar di tempat ke arah belokan terakhir,
//                            sampai salah satu sensor menemukan garis lagi
//
// Pin (sama persis dengan schematic.png):
//   Sensor kiri  OUT -> GPIO32, VCC -> 3V3, GND -> GND
//   Sensor kanan OUT -> GPIO33, VCC -> 3V3, GND -> GND
//   L298N ENA -> GPIO25 (PWM motor kiri)
//   L298N IN1 -> GPIO26, IN2 -> GPIO27 (arah motor kiri)
//   L298N IN3 -> GPIO18, IN4 -> GPIO19 (arah motor kanan)
//   L298N ENB -> GPIO13 (PWM motor kanan)
//   L298N 12V -> baterai 7,4 V, GND -> GND, 5V -> VIN ESP32
//   LED bawaan GPIO2 menyala saat robot sedang mengikuti garis.
//
// Perhatian daya: baterai 7,4 V HANYA masuk ke pin 12V L298N.
// Jangan hubungkan baterai langsung ke VIN ESP32 — ESP32 dicatu
// dari regulator 5V L298N lewat VIN.

// ---------- Pin ----------
const uint8_t PIN_SENSOR_KIRI  = 32;  // OUT sensor garis kiri (TCRT5000)
const uint8_t PIN_SENSOR_KANAN = 33;  // OUT sensor garis kanan (TCRT5000)
const uint8_t PIN_ENA = 25;           // PWM motor kiri (L298N ENA)
const uint8_t PIN_IN1 = 26;           // Arah motor kiri
const uint8_t PIN_IN2 = 27;           // Arah motor kiri
const uint8_t PIN_IN3 = 18;           // Arah motor kanan
const uint8_t PIN_IN4 = 19;           // Arah motor kanan
const uint8_t PIN_ENB = 13;           // PWM motor kanan (L298N ENB)
const uint8_t PIN_LED  = 2;            // LED bawaan ESP32

// ---------- Konstanta ----------
const uint16_t INTERVAL_BACA_MS = 10;    // baca sensor tiap 10 ms
const uint16_t FREKUENSI_PWM_HZ = 5000;  // frekuensi PWM motor
const uint8_t  RESOLUSI_PWM_BIT = 8;     // PWM 8-bit (0-255)
const uint8_t  KECEPATAN_MAJU   = 180;   // kecepatan lurus (0-255)
const uint8_t  KECEPATAN_BELOK  = 110;   // kecepatan motor sisi dalam saat belok
const uint8_t  KECEPATAN_PUTAR  = 150;   // kecepatan putar mencari garis
const uint8_t  PWM_MAKSIMAL     = 255;   // nilai PWM tertinggi

// Arah putar terakhir: true = terakhir belok kiri, false = kanan.
// Dipakai saat robot kehilangan garis supaya putarnya tidak asal.
bool terakhirBelokKiri = true;

unsigned long waktuBacaTerakhir = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SENSOR_KIRI, INPUT);
  pinMode(PIN_SENSOR_KANAN, INPUT);
  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  // PWM motor (ESP32 core 4.x: ledcAttach mengikat pin ke channel)
  ledcAttach(PIN_ENA, FREKUENSI_PWM_HZ, RESOLUSI_PWM_BIT);
  ledcAttach(PIN_ENB, FREKUENSI_PWM_HZ, RESOLUSI_PWM_BIT);

  berhenti();
  Serial.println("Robot line follower siap.");
}

void loop() {
  unsigned long sekarang = millis();
  if (sekarang - waktuBacaTerakhir < INTERVAL_BACA_MS) {
    return;  // belum waktunya baca sensor lagi
  }
  waktuBacaTerakhir = sekarang;

  bool kiriDiGaris  = digitalRead(PIN_SENSOR_KIRI) == HIGH;
  bool kananDiGaris = digitalRead(PIN_SENSOR_KANAN) == HIGH;

  if (kiriDiGaris && kananDiGaris) {
    majuLurus();
  } else if (kiriDiGaris) {
    belokKiri();
  } else if (kananDiGaris) {
    belokKanan();
  } else {
    putarCariGaris();
  }
}

// ---------- Gerakan ----------

// Atur satu motor: kecepatan 0-255, maju = true berarti roda ke depan.
void aturMotor(uint8_t pinPwm, uint8_t pinArah1, uint8_t pinArah2,
               uint8_t kecepatan, bool maju) {
  digitalWrite(pinArah1, maju ? HIGH : LOW);
  digitalWrite(pinArah2, maju ? LOW : HIGH);
  ledcWrite(pinPwm, kecepatan);
}

void majuLurus() {
  aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, KECEPATAN_MAJU, true);
  aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, KECEPATAN_MAJU, true);
  digitalWrite(PIN_LED, HIGH);
  Serial.println("Maju lurus");
}

void belokKiri() {
  // Motor kiri pelan, motor kanan tetap cepat -> robot belok kiri.
  aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, KECEPATAN_BELOK, true);
  aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, KECEPATAN_MAJU, true);
  terakhirBelokKiri = true;
  digitalWrite(PIN_LED, HIGH);
  Serial.println("Belok kiri");
}

void belokKanan() {
  aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, KECEPATAN_MAJU, true);
  aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, KECEPATAN_BELOK, true);
  terakhirBelokKiri = false;
  digitalWrite(PIN_LED, HIGH);
  Serial.println("Belok kanan");
}

void putarCariGaris() {
  // Putar di tempat ke arah belokan terakhir sampai garis ketemu lagi.
  if (terakhirBelokKiri) {
    aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, KECEPATAN_PUTAR, false);
    aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, KECEPATAN_PUTAR, true);
  } else {
    aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, KECEPATAN_PUTAR, true);
    aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, KECEPATAN_PUTAR, false);
  }
  digitalWrite(PIN_LED, LOW);
  Serial.println("Mencari garis...");
}

void berhenti() {
  aturMotor(PIN_ENA, PIN_IN1, PIN_IN2, 0, true);
  aturMotor(PIN_ENB, PIN_IN3, PIN_IN4, 0, true);
  digitalWrite(PIN_LED, LOW);
}
