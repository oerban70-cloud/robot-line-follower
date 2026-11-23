# Robot Line Follower — ESP32 + 2 Sensor Garis TCRT5000 + Driver L298N

Robot kecil yang mengikuti garis hitam di lantai putih tanpa disentuh
sama sekali. Dua sensor garis TCRT5000 di depan menjadi "mata" robot:
begitu garis menikung, ESP32 langsung membelokkan robot mengikuti
garisnya. Cocok untuk belajar sensor, PWM, dan logika kontrol sederhana.

Video penjelasan (60 detik):
link menyusul setelah upload

## Komponen

| Bagian | Keterangan |
|---|---|
| ESP32 DevKit | Otak robot |
| Sensor garis TCRT5000 (2x) | Baca garis hitam, output digital HIGH di atas garis |
| Driver motor L298N | Atur arah + kecepatan 2 motor DC (PWM) |
| Motor DC + roda (2x) | Penggerak kiri dan kanan |
| Baterai 7,4 V (2x 18650 seri) | Catu daya motor lewat L298N |
| LED bawaan GPIO2 | Menyala saat robot sedang mengikuti garis |

## Wiring

| Dari | Ke |
|---|---|
| Sensor kiri OUT | ESP32 GPIO32 |
| Sensor kanan OUT | ESP32 GPIO33 |
| Sensor VCC (keduanya) | ESP32 3V3 |
| L298N ENA | ESP32 GPIO25 (PWM motor kiri) |
| L298N IN1 | ESP32 GPIO26 |
| L298N IN2 | ESP32 GPIO27 |
| L298N IN3 | ESP32 GPIO18 |
| L298N IN4 | ESP32 GPIO19 |
| L298N ENB | ESP32 GPIO13 (PWM motor kanan) |
| L298N OUT1 / OUT2 | Motor DC kiri (+ / -) |
| L298N OUT3 / OUT4 | Motor DC kanan (+ / -) |
| L298N 12V | Baterai 7,4 V (+) |
| L298N 5V | ESP32 VIN |
| Semua GND | Digabung (sensor, ESP32, L298N, baterai -) |

Skema lengkap: [schematic.png](schematic.png)

Perhatian daya: baterai 7,4 V HANYA masuk ke pin 12V L298N.
Jangan hubungkan baterai langsung ke VIN ESP32 — ESP32 dicatu dari
regulator 5V L298N lewat VIN.

## Cara kerja

Dua sensor membaca garis tiap 10 milidetik. Kombinasi bacaannya
menentukan gerakan robot:

| Sensor kiri | Sensor kanan | Gerakan |
|---|---|---|
| HIGH (di garis) | HIGH (di garis) | Maju lurus, kedua motor kecepatan 180 |
| HIGH (di garis) | LOW | Belok kiri — motor kiri pelan (110), kanan tetap cepat |
| LOW | HIGH (di garis) | Belok kanan — motor kanan pelan (110), kiri tetap cepat |
| LOW | LOW (garis hilang) | Putar di tempat ke arah belokan terakhir sampai garis ketemu lagi |

Kecepatan motor diatur dengan PWM 5 kHz 8-bit lewat pin ENA/ENB
L298N. LED bawaan GPIO2 menyala selama robot mengikuti garis dan
padam saat robot sedang mencari garis.

## Konfigurasi (di awal sketch)

- `INTERVAL_BACA_MS` (10) — seberapa sering sensor dibaca.
- `FREKUENSI_PWM_HZ` (5000) dan `RESOLUSI_PWM_BIT` (8) — sinyal PWM motor.
- `KECEPATAN_MAJU` (180) — kecepatan lurus, 0–255.
- `KECEPATAN_BELOK` (110) — kecepatan motor sisi dalam saat belok.
- `KECEPATAN_PUTAR` (150) — kecepatan putar saat mencari garis.

Kalau robot terlalu liar di tikungan, turunkan `KECEPATAN_MAJU`;
kalau robot lambat merespons, turunkan `INTERVAL_BACA_MS`.

## Library yang dibutuhkan

Tidak ada — sketch ini hanya memakai fungsi bawaan ESP32 core
(`digitalRead`, `ledcAttach`, `ledcWrite`). Tidak perlu install
library tambahan.

## Compile

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 RobotLineFollower
```

Terverifikasi 23 Nov 2026: sketch 300.491 byte (22% flash),
RAM 23.396 byte (7%), ESP32 core 4.0.0.

## Lisensi

MIT — lihat [LICENSE](LICENSE).
