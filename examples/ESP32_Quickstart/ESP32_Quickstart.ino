/**
 * Contoh ESP32 — Zynq IoT Platform (DX Sempurna, Nol Magic).
 * 
 * 1. Zynq.begin(ssid, password, projectId, deviceId, deviceToken)
 * 2. Zynq.on(V2, kontrolRelay)    --> Aksi saat tombol di Dashboard ditekan.
 * 3. Zynq.every(2000, bacaSensor) --> Rutinitas kirim sensor berkala (otomatis dibatch).
 * 4. Zynq.run()                  --> CUKUP 1 BARIS di loop()! Tanpa millis() / delay().
 */

#include <WiFi.h>
#include <Zynq.h>

// 1. Konfigurasi WiFi
const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI";

// 2. Kredensial Perangkat Zynq (Salin dari Dashboard: https://zynq.epsitech.id)
const char* projectId   = "prj_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceId    = "dev_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceToken = "zxt_abcdef0123456789...";

const int LED_PIN = 2; // Pin LED bawaan ESP32

// ==========================================
// FUNGSI AKTUATOR (Menerima input dari Dashboard)
// ==========================================
void kontrolRelay(int status) {
  digitalWrite(LED_PIN, status ? HIGH : LOW);
  Serial.print("Lampu diubah dari Dashboard Zynq: ");
  Serial.println(status ? "MENYALA" : "PADAM");
}

// ==========================================
// FUNGSI SENSOR (Dipanggil otomatis tiap 2 detik)
// ==========================================
void bacaSensor() {
  float suhu = 28.5 + (random(-10, 10) / 10.0);
  float hum  = 65.0 + (random(-15, 15) / 10.0);

  // Bebas print ke Serial Monitor untuk debugging lokal
  Serial.printf("[Sensor] Suhu: %.1f °C | Kelembaban: %.1f %%\n", suhu, hum);

  // Kirim ke Zynq! Nilai otomatis digabung jadi 1 paket JSON hemat bandwidth
  Zynq.send(V0, suhu);
  Zynq.send(V1, hum);
}

void setup() {
  Serial.begin(115200);

  // pinMode fisik tetap milik programmer
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Hubungkan ke WiFi dan Zynq Cloud dengan kredensial lengkap
  Zynq.begin(ssid, password, projectId, deviceId, deviceToken);

  // Daftarkan event perintah aktuator (Cloud -> ESP32)
  Zynq.on(V2, kontrolRelay);

  // Jadwalkan pembacaan sensor berkala tiap 2 detik (2000 ms)
  Zynq.every(2000, bacaSensor);

  // Beri nama VPin secara praktis (satuan & rentang diatur di Dashboard Zynq)
  Zynq.setLabel(V0, "Suhu Ruangan");
  Zynq.setLabel(V1, "Kelembaban");
  Zynq.setLabel(V2, "Lampu Utama");
}

void loop() {
  // CUKUP 1 BARIS INI! Zynq mengurus koneksi, keepalive, dan mengeksekusi every(2000).
  Zynq.run();
}
