<div align="center">

# ⚡ Zynq IoT — Arduino C++ Library

**Official, high-performance, and developer-friendly IoT library for ESP32 and ESP8266.**  
Connect your hardware to the [**Zynq IoT Cloud Platform (zynq.epsitech.id)**](https://zynq.epsitech.id) with zero friction, zero magic, and instant auto-discovery.

[![Platform](https://img.shields.io/badge/Platform-zynq.epsitech.id-7C3AED?logo=google-cloud&logoColor=white)](https://zynq.epsitech.id)
[![Arduino Library](https://img.shields.io/badge/Arduino-Library-00979C?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![ESP32](https://img.shields.io/badge/Hardware-ESP32-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![ESP8266](https://img.shields.io/badge/Hardware-ESP8266-41B883?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Release](https://img.shields.io/badge/Version-1.0.0-emerald)](https://github.com/epsilon-technology/zynq-arduino)

[🌐 Web Platform](https://zynq.epsitech.id) •
[Fitur Utama](#-fitur-utama) •
[Cara Instalasi](#-cara-instalasi) •
[Contoh Cepat](#-contoh-cepat-esp32) •
[Panduan API](#-panduan-lengkap-api) •
[Troubleshooting](#-pemecahan-masalah)

---

</div>

## 💡 Filosofi Desain (Developer Experience / DX)

Sebagian besar library IoT memaksakan terlalu banyak *macro magic* yang membingungkan atau mengharuskan kodingan rumit puluhan baris hanya untuk membaca sensor. **Zynq hadir dengan pendekatan berbeda:**

- **Nol Magic:** Callback fungsi C++ biasa yang mudah dibaca, bisa di-`Serial.println()`, dan bisa dipanggil dari mana saja.
- **Auto-Discovery VPin:** Cukup koding VPin di firmware Anda; Dashboard Zynq langsung mendeteksi pin tersebut secara otomatis.
- **Auto-Role Inference:**
  - `Zynq.on(V2, ...)` $\rightarrow$ Otomatis terdaftar sebagai **Kontrol / Aktuator** (*Read-Write*).
  - `Zynq.send(V0, ...)` $\rightarrow$ Otomatis terdaftar sebagai **Sensor Telemetri** (*Read-Only*).
- **Auto-Batching Hemat Kuota:** Pengiriman data di dalam `Zynq.every()` otomatis dikemas menjadi satu paket JSON tunggal.
- **Loop Bersih:** Tidak ada variabel `millis()` bertumpuk dan tidak ada `delay()` yang mematikan koneksi. Cukup `Zynq.run();`.

---

## 📦 Cara Instalasi

### 0. Dapatkan Kredensial di Dashboard Zynq
1. Buka dan login ke platform di [**https://zynq.epsitech.id**](https://zynq.epsitech.id).
2. Buat Project & tambahkan Perangkat baru untuk mendapatkan `projectId`, `deviceId`, dan `deviceToken`.
3. Pada halaman detail perangkat (tab **Hubungkan**), Anda juga bisa langsung menyalin sketsa kode yang sudah terisi otomatis!

---

### 1. Prasyarat: Pasang Library `PubSubClient`
Library Zynq membutuhkan dependensi MQTT **PubSubClient** oleh *Nick O'Leary*:
1. Buka **Arduino IDE**.
2. Masuk ke menu **Sketch** $\rightarrow$ **Include Library** $\rightarrow$ **Manage Libraries...** (atau tekan `Ctrl + Shift + I`).
3. Cari **`PubSubClient`** dan klik tombol **Install**.

---

### 2. Pasang Library Zynq (.ZIP)
1. Unduh repositori ini sebagai file ZIP:  
   👉 [**Download Zynq-Arduino (.ZIP)**](https://github.com/epsilon-technology/zynq-arduino/archive/refs/heads/main.zip)
2. Di Arduino IDE, buka menu:  
   $$\text{Sketch} \rightarrow \text{Include Library} \rightarrow \text{Add .ZIP Library...}$$
3. Pilih file ZIP yang baru Anda unduh.
4. Selesai! Contoh kode resmi dapat langsung dibuka via:  
   $$\text{File} \rightarrow \text{Examples} \rightarrow \text{ZynqIoT} \rightarrow \text{ESP32_Quickstart}$$

---

## 🚀 Contoh Cepat (ESP32)

Kodingan di bawah menghubungkan ESP32 ke WiFi, membaca sensor suhu & kelembaban setiap 2 detik, serta mengontrol lampu relay dari dashboard web Zynq:

```cpp
#include <WiFi.h>
#include <Zynq.h>

// 1. Konfigurasi WiFi
const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI";

// 2. Kredensial Perangkat Zynq (Salin dari Dashboard https://zynq.epsitech.id)
const char* projectId   = "prj_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceId    = "dev_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceToken = "zxt_abcdef0123456789...";

const int RELAY_PIN = 2; // Pin fisik relay / LED pada board

// ==========================================
// 1. FUNGSI AKTUATOR (Menerima input dari Dashboard Zynq)
// ==========================================
void kontrolRelay(int status) {
  digitalWrite(RELAY_PIN, status ? HIGH : LOW);
  
  // Bebas Serial.println() untuk kebutuhan debugging:
  Serial.print("[Relay] Lampu diubah jadi: ");
  Serial.println(status ? "MENYALA" : "PADAM");
}

// ==========================================
// 2. FUNGSI SENSOR (Dipanggil otomatis oleh Zynq berkala)
// ==========================================
void bacaSensor() {
  float suhu = 28.5 + (random(-10, 10) / 10.0);
  float hum  = 65.0 + (random(-15, 15) / 10.0);

  Serial.printf("[Sensor] Suhu: %.1f C | Hum: %.1f %%\n", suhu, hum);

  // Kirim data ke VPin (Otomatis digabung jadi 1 paket hemat transmisi!):
  Zynq.send(V0, suhu);
  Zynq.send(V1, hum);
}

// ==========================================
// 3. SETUP (Registrasi Alami & Eksplisit)
// ==========================================
void setup() {
  Serial.begin(115200);

  // Inisialisasi pin hardware fisik tetap normal milik Anda
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Hubungkan ke WiFi & Zynq Cloud (Port 8883 TLS) dengan kredensial lengkap:
  Zynq.begin(ssid, password, projectId, deviceId, deviceToken);

  // Daftarkan listener perintah aktuator (Cloud -> ESP32)
  Zynq.on(V2, kontrolRelay);

  // Jadwalkan pembacaan sensor berkala tiap 2 detik (2000 ms)
  Zynq.every(2000, bacaSensor);

  // Beri nama label VPin (opsional, satuan & rentang diatur di Dashboard)
  Zynq.setLabel(V0, "Suhu Ruangan");
  Zynq.setLabel(V1, "Kelembaban");
  Zynq.setLabel(V2, "Lampu Utama");
}

// ==========================================
// 4. LOOP (Cukup 1 Baris!)
// ==========================================
void loop() {
  // Menjaga koneksi MQTT, keepalive ping, dan eksekusi every()
  Zynq.run();
}
```

---

## 📖 Panduan Lengkap API

### 1. `Zynq.begin(ssid, password, projectId, deviceId, deviceToken)`
Menginisialisasi modul WiFi, enkripsi TLS port 8883, dan autentikasi token perangkat ke Zynq Broker.
```cpp
Zynq.begin(ssid, password, projectId, deviceId, deviceToken);
```

### 2. `Zynq.on(pin, callback)`
Mendaftarkan fungsi C++ yang akan dipanggil setiap kali tombol/switch/slider di Dashboard Zynq ditekan. Mendukung tipe parameter yang fleksibel:
```cpp
// A. Menerima status integer/boolean (cocok untuk saklar/relay)
void relayHandler(int status) { ... }
Zynq.on(V1, relayHandler);

// B. Menerima nilai desimal float (cocok untuk slider)
void dimmerHandler(float nilai) { ... }
Zynq.on(V2, dimmerHandler);

// C. Menerima objek ZynqParam (untuk kontrol multi-tipe lanjutan)
void customHandler(ZynqParam param) {
  bool b = param.asBool();
  float f = param.asFloat();
}
Zynq.on(V3, customHandler);
```

### 3. `Zynq.every(intervalMs, callback)`
Menjadwalkan fungsi sensor berkala tanpa pernah memblokir koneksi jaringan.
```cpp
Zynq.every(1000, bacaSensorCepat);  // Tiap 1 detik
Zynq.every(5000, bacaSuhuRuangan);   // Tiap 5 detik
Zynq.every(60000, cekBaterai);       // Tiap 1 menit
```
> **Keunggulan Batching:** Setiap pemanggilan `Zynq.send()` di dalam fungsi `every()` otomatis dikemas menjadi satu pesan JSON tunggal (`{"v0": 28.5, "v1": 65.0}`).

### 4. `Zynq.send(pin, value)`
Mengirim data sensor ke Virtual Pin.
```cpp
Zynq.send(V0, 28.5);        // Angka desimal (float/double)
Zynq.send(V1, 42);          // Angka bulat (int)
Zynq.send(V2, true);        // Boolean (true/false)
Zynq.send("v0", 28.5);      // Mendukung penulisan string "v0"
```
*(Tersedia juga alias `Zynq.virtualWrite(...)` bagi yang terbiasa dengan sintaks Blynk).*

### 5. `Zynq.setLabel(pin, "Label")`
Memberikan nama manusiawi untuk VPin agar langsung terbaca rapi di Dashboard Zynq saat pertama kali terhubung.
```cpp
Zynq.setLabel(V0, "Suhu Ruangan");
Zynq.setLabel(V1, "Kelembaban");
```

### 6. `Zynq.run()`
Menjaga koneksi MQTT tetap aktif, mengirim sinyal *keep-alive ping*, menerima perintah masuk, dan mengeksekusi jadwal `every()`. Wajib diletakkan di dalam `void loop()`.

---

## 🛠️ Pemecahan Masalah (Troubleshooting)

### Q: Mengapa Serial Monitor menampilkan pesan `rc=-2`?
* **Penyebab:** Koneksi jaringan internet gagal menjangkau broker (WiFi terputus atau timeout).
* **Solusi:** Pastikan nama WiFi (`ssid`) dan kata sandi (`password`) sudah benar, dan hotspot internet Anda dapat mengakses port internet keluar (port 8883).

### Q: Mengapa status alat di Dashboard tetap "Offline"?
* **Penyebab:** Token yang dimasukkan tidak cocok atau sudah diputar (*rotated*).
* **Solusi:** Buka Dashboard Zynq $\rightarrow$ Masuk ke detail perangkat $\rightarrow$ Salin token perangkat yang aktif dan masukkan ke kode `#define ZYNQ_TOKEN`.

### Q: Bisakah saya tetap menggunakan tombol fisik di breadboard?
* **Tentu saja!** Anda tetap bisa membaca tombol fisik dengan `digitalRead()` biasa di dalam `void loop()`. Saat tombol fisik ditekan, Anda bisa memanggil `Zynq.send(V2, status)` untuk mengabari dashboard secara instan!

---

## 📄 Lisensi

Library ini dirilis di bawah lisensi **Apache-2.0**. Bebas digunakan untuk proyek hobi, edukasi, maupun komersial berbasis platform Zynq IoT.
