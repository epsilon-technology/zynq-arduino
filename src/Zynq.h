/**
 * Zynq.h — Library Resmi Arduino & ESP32/ESP8266 untuk Zynq IoT Cloud Platform.
 * 
 * Pengalaman Pengembang (DX Sempurna, Nol Magic):
 * 1. Zynq.on(V1, fungsi)    --> Tangani perintah saklar/slider dari dashboard (Aktuator).
 * 2. Zynq.every(2000, fungsi)--> Rutinitas pembacaan sensor berkala tanpa millis()/delay manual.
 * 3. Zynq.send(V0, nilai)   --> Kirim data sensor (otomatis dibatch hemat kuota saat di every).
 * 4. Zynq.setLabel(V0, "Nama")--> Beri nama VPin secara bersih (satuan diatur di Dashboard).
 * 5. Zynq.run()             --> Cukup 1 baris di void loop() menangani seluruh jaringan.
 */

#ifndef ZYNQ_H
#define ZYNQ_H

#include <Arduino.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h>
#else
  #error "Platform tidak didukung! Library Zynq IoT saat ini mendukung ESP32 dan ESP8266."
#endif

#include <PubSubClient.h>

// Definisi Konstanta VPin V0 .. V63
enum ZynqVPin : uint8_t {
  V0 = 0, V1, V2, V3, V4, V5, V6, V7, V8, V9,
  V10, V11, V12, V13, V14, V15, V16, V17, V18, V19,
  V20, V21, V22, V23, V24, V25, V26, V27, V28, V29,
  V30, V31, V32, V33, V34, V35, V36, V37, V38, V39,
  V40, V41, V42, V43, V44, V45, V46, V47, V48, V49,
  V50, V51, V52, V53, V54, V55, V56, V57, V58, V59,
  V60, V61, V62, V63
};

/**
 * Parameter nilai dari Dashboard Zynq ke ESP.
 * Helper: param.asBool(), param.asInt(), param.asFloat(), param.asDouble()
 */
class ZynqParam {
public:
  double numVal = 0.0;
  bool boolVal = false;
  bool isBoolean = false;

  bool asBool() const { return isBoolean ? boolVal : (numVal != 0.0); }
  int asInt() const { return (int)numVal; }
  long asLong() const { return (long)numVal; }
  float asFloat() const { return (float)numVal; }
  double asDouble() const { return numVal; }
  String asString() const {
    if (isBoolean) return boolVal ? "true" : "false";
    return String(numVal);
  }
};

// Tipe callback fleksibel: bisa terima (int), (float), atau (ZynqParam)
typedef void (*ZynqIntHandler)(int value);
typedef void (*ZynqFloatHandler)(float value);
typedef void (*ZynqParamHandler)(ZynqParam param);
typedef void (*ZynqTaskCallback)();

struct ZynqActionEntry {
  uint8_t pin;
  enum HandlerType { TYPE_INT, TYPE_FLOAT, TYPE_PARAM } type;
  union {
    ZynqIntHandler intFn;
    ZynqFloatHandler floatFn;
    ZynqParamHandler paramFn;
  };
};

struct ZynqScheduledTask {
  unsigned long interval;
  unsigned long lastRun;
  ZynqTaskCallback callback;
  bool enabled;
};

/**
 * Metadata VPin internal untuk auto-discovery
 */
struct ZynqPinMeta {
  uint8_t pin;
  String label;
  String type; // "float", "int", "bool"
  bool readOnly;
  bool declared;
};

class ZynqClass {
public:
  ZynqClass();

  /**
   * Inisialisasi koneksi Zynq Cloud
   */
  void begin(
    const char* ssid,
    const char* wifiPassword,
    const char* projectId,
    const char* deviceId,
    const char* deviceToken,
    const char* brokerHost = "zynq-mqtt.epsitech.id",
    uint16_t brokerPort = 8883
  );

  /**
   * 1. Zynq.on(pin, fungsi)
   * Daftarkan fungsi untuk menangani perintah dari Dashboard (Aktuator).
   * Otomatis menandai VPin sebagai Kontrol di Dashboard!
   * 
   * Mendukung fungsi berparameter:
   * - void lampu(int status)
   * - void dimmer(float nilai)
   * - void kontrol(ZynqParam param)
   */
  void on(uint8_t pin, ZynqIntHandler handler);
  void on(uint8_t pin, ZynqFloatHandler handler);
  void on(uint8_t pin, ZynqParamHandler handler);

  void on(const char* pinStr, ZynqIntHandler handler)   { on(parsePinNumber(pinStr), handler); }
  void on(const char* pinStr, ZynqFloatHandler handler) { on(parsePinNumber(pinStr), handler); }
  void on(const char* pinStr, ZynqParamHandler handler) { on(parsePinNumber(pinStr), handler); }

  /**
   * 2. Zynq.every(intervalMs, fungsi)
   * Jalankan fungsi pembacaan sensor berkala tanpa perlu repot bikin millis()/delay manual.
   * Seluruh Zynq.send() di dalam fungsi ini otomatis digabung (batch) jadi 1 paket hemat kuota!
   */
  int every(unsigned long intervalMs, ZynqTaskCallback callback);

  /**
   * 3. Zynq.send(pin, nilai)
   * Kirim nilai sensor ke VPin Zynq.
   * Otomatis menandai VPin sebagai Sensor Telemetri di Dashboard!
   */
  bool send(uint8_t pin, double value);
  bool send(uint8_t pin, float value) { return send(pin, (double)value); }
  bool send(uint8_t pin, int value) { return send(pin, (double)value); }
  bool send(uint8_t pin, long value) { return send(pin, (double)value); }
  bool send(uint8_t pin, bool value);

  bool send(const char* pinStr, double value);
  bool send(const char* pinStr, float value) { return send(pinStr, (double)value); }
  bool send(const char* pinStr, int value) { return send(pinStr, (double)value); }
  bool send(const char* pinStr, bool value);

  // Kompatibilitas sintaks virtualWrite
  bool virtualWrite(uint8_t pin, double value) { return send(pin, value); }
  bool virtualWrite(uint8_t pin, float value)  { return send(pin, value); }
  bool virtualWrite(uint8_t pin, int value)    { return send(pin, value); }
  bool virtualWrite(uint8_t pin, bool value)   { return send(pin, value); }
  bool virtualWrite(const char* pinStr, double value) { return send(pinStr, value); }
  bool virtualWrite(const char* pinStr, float value)  { return send(pinStr, value); }
  bool virtualWrite(const char* pinStr, int value)    { return send(pinStr, value); }
  bool virtualWrite(const char* pinStr, bool value)   { return send(pinStr, value); }

  /**
   * 4. Zynq.setLabel(pin, "Nama")
   * Beri nama manusiawi untuk VPin (opsional).
   * Satuan, ikon, dan rentang diatur langsung di Dashboard Zynq.
   */
  void setLabel(uint8_t pin, const char* label);
  void setLabel(const char* pinStr, const char* label);

  /**
   * 5. Zynq.run()
   * Cukup 1 baris ini di void loop()! Mengurus koneksi, keepalive, dan mengeksekusi every().
   */
  void run();
  void loop() { run(); }

  bool isConnected();
  void syncVpins();

private:
  const char* _ssid = nullptr;
  const char* _wifiPassword = nullptr;
  String _projectId;
  String _deviceId;
  String _deviceToken;
  String _brokerHost;
  uint16_t _brokerPort = 8883;

  String _topicData;
  String _topicCmd;
  String _topicAck;
  String _topicStatus;

  WiFiClientSecure _tlsClient;
  PubSubClient _mqtt;

  unsigned long _lastReconnectAttempt = 0;
  uint32_t _seq = 0;
  bool _vpinsSynced = false;

  bool _isBatching = false;
  String _batchBuffer;
  size_t _batchItemCount = 0;

  // Daftar action listener Zynq.on(...)
  static const size_t MAX_ACTIONS = 64;
  ZynqActionEntry _actions[MAX_ACTIONS];
  size_t _actionCount = 0;

  // Daftar scheduled tasks Zynq.every(...)
  static const size_t MAX_TASKS = 16;
  ZynqScheduledTask _tasks[MAX_TASKS];
  size_t _taskCount = 0;

  // Metadata VPin
  ZynqPinMeta _meta[64];
  size_t _metaCount = 0;

  void ensureWifi();
  bool ensureMqtt();
  void handleMessage(char* topic, byte* payload, unsigned int length);
  static void staticCallback(char* topic, byte* payload, unsigned int length);
  static ZynqClass* _instance;
  uint64_t currentTimestamp();

  void registerAction(uint8_t pin, ZynqActionEntry::HandlerType type, void* fn);
  ZynqPinMeta* findOrCreateMeta(uint8_t pin);
  uint8_t parsePinNumber(const char* pinStr);
};

// Global singleton Zynq (seperti Serial / WiFi)
extern ZynqClass Zynq;

#endif // ZYNQ_H
