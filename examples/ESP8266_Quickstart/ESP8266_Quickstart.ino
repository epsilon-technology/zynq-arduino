/**
 * Contoh ESP8266 (NodeMCU / Wemos D1 Mini) — Zynq IoT Platform (DX Sempurna, Nol Magic).
 */

#include <ESP8266WiFi.h>
#include <Zynq.h>

const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI";

const char* projectId   = "prj_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceId    = "dev_01M48RGF7P60NYEEXVF9KJ8KCK";
const char* deviceToken = "zxt_abcdef0123456789...";

const int RELAY_PIN = D1;

// 1. Aksi saat tombol V1 di Dashboard ditekan
void kontrolRelay(int status) {
  digitalWrite(RELAY_PIN, status ? HIGH : LOW);
  Serial.print("Relay: ");
  Serial.println(status ? "ON" : "OFF");
}

// 2. Pembacaan sensor berkala
void bacaSensor() {
  float suhu = 29.2 + (random(-10, 10) / 10.0);
  Serial.printf("[Sensor] Suhu: %.1f\n", suhu);
  Zynq.send(V0, suhu);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Hubungkan ke WiFi dan Zynq Cloud
  Zynq.begin(ssid, password, projectId, deviceId, deviceToken);

  Zynq.on(V1, kontrolRelay);
  Zynq.every(3000, bacaSensor); // Kirim tiap 3 detik

  Zynq.setLabel(V0, "Suhu Udara");
  Zynq.setLabel(V1, "Relay Pompa");
}

void loop() {
  Zynq.run();
}
