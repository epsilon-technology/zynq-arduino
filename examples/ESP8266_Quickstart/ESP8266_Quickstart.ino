/**
 * Contoh ESP8266 (NodeMCU / Wemos D1 Mini) — Zynq IoT Platform (DX Sempurna, Nol Magic).
 */

#define ZYNQ_PROJECT "prj_01M48RGF7P60NYEEXVF9KJ8KCK"
#define ZYNQ_DEVICE  "dev_01M48RGF7P60NYEEXVF9KJ8KCK"
#define ZYNQ_TOKEN   "zxt_abcdef0123456789..."

#include <ESP8266WiFi.h>
#include <Zynq.h>

const char* ssid     = "NAMA_WIFI_ANDA";
const char* password = "PASSWORD_WIFI";

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

  Zynq.begin(ssid, password);

  Zynq.on(V1, kontrolRelay);
  Zynq.every(3000, bacaSensor); // Kirim tiap 3 detik

  Zynq.setLabel(V0, "Suhu Udara");
  Zynq.setLabel(V1, "Relay Pompa");
}

void loop() {
  Zynq.run();
}
