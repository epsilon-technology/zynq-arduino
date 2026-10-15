#include "Zynq.h"
#include <time.h>

ZynqClass* ZynqClass::_instance = nullptr;
ZynqClass Zynq; // Global instance siap pakai

ZynqClass::ZynqClass() : _mqtt(_tlsClient) {
  _instance = this;
}

void ZynqClass::begin(
  const char* ssid,
  const char* wifiPassword,
  const char* projectId,
  const char* deviceId,
  const char* deviceToken,
  const char* brokerHost,
  uint16_t brokerPort
) {
  _ssid = ssid;
  _wifiPassword = wifiPassword;
  _projectId = projectId ? projectId : "";
  _deviceId = deviceId ? deviceId : "";
  _deviceToken = deviceToken ? deviceToken : "";
  _brokerHost = brokerHost;
  _brokerPort = brokerPort;

  _topicData   = "v1/" + _projectId + "/" + _deviceId + "/data";
  _topicCmd    = "v1/" + _projectId + "/" + _deviceId + "/cmd";
  _topicAck    = "v1/" + _projectId + "/" + _deviceId + "/ack";
  _topicStatus = "v1/" + _projectId + "/" + _deviceId + "/status";

  #if defined(ESP32) || defined(ESP8266)
    _tlsClient.setInsecure();
  #endif

  _mqtt.setBufferSize(2048);
  _mqtt.setServer(_brokerHost.c_str(), _brokerPort);
  _mqtt.setCallback(ZynqClass::staticCallback);

  configTime(0, 0, "pool.ntp.org", "time.google.com");

  ensureWifi();
}

void ZynqClass::registerAction(uint8_t pin, ZynqActionEntry::HandlerType type, void* fn) {
  if (_actionCount >= MAX_ACTIONS || fn == nullptr) return;

  // Cari apakah pin sudah ada
  for (size_t i = 0; i < _actionCount; i++) {
    if (_actions[i].pin == pin) {
      _actions[i].type = type;
      if (type == ZynqActionEntry::TYPE_INT) _actions[i].intFn = (ZynqIntHandler)fn;
      else if (type == ZynqActionEntry::TYPE_FLOAT) _actions[i].floatFn = (ZynqFloatHandler)fn;
      else _actions[i].paramFn = (ZynqParamHandler)fn;
      return;
    }
  }

  // Daftarkan aksi baru
  _actions[_actionCount].pin = pin;
  _actions[_actionCount].type = type;
  if (type == ZynqActionEntry::TYPE_INT) _actions[_actionCount].intFn = (ZynqIntHandler)fn;
  else if (type == ZynqActionEntry::TYPE_FLOAT) _actions[_actionCount].floatFn = (ZynqFloatHandler)fn;
  else _actions[_actionCount].paramFn = (ZynqParamHandler)fn;
  _actionCount++;

  // Otomatis tandai VPin sebagai Kontrol / Aktuator (readOnly: false)
  ZynqPinMeta* m = findOrCreateMeta(pin);
  if (m) {
    m->readOnly = false;
    m->type = (type == ZynqActionEntry::TYPE_FLOAT) ? "float" : "bool";
    if (m->label.length() == 0) {
      m->label = "Kontrol " + String(pin);
    }
  }
}

void ZynqClass::on(uint8_t pin, ZynqIntHandler handler) {
  registerAction(pin, ZynqActionEntry::TYPE_INT, (void*)handler);
}

void ZynqClass::on(uint8_t pin, ZynqFloatHandler handler) {
  registerAction(pin, ZynqActionEntry::TYPE_FLOAT, (void*)handler);
}

void ZynqClass::on(uint8_t pin, ZynqParamHandler handler) {
  registerAction(pin, ZynqActionEntry::TYPE_PARAM, (void*)handler);
}

int ZynqClass::every(unsigned long intervalMs, ZynqTaskCallback callback) {
  if (_taskCount >= MAX_TASKS || callback == nullptr) return -1;

  _tasks[_taskCount].interval = intervalMs;
  _tasks[_taskCount].lastRun = millis();
  _tasks[_taskCount].callback = callback;
  _tasks[_taskCount].enabled = true;
  return _taskCount++;
}

ZynqPinMeta* ZynqClass::findOrCreateMeta(uint8_t pin) {
  for (size_t i = 0; i < _metaCount; i++) {
    if (_meta[i].pin == pin) return &_meta[i];
  }
  if (_metaCount < 64) {
    _meta[_metaCount].pin = pin;
    _meta[_metaCount].label = "";
    _meta[_metaCount].type = "float";
    _meta[_metaCount].readOnly = true;
    _meta[_metaCount].declared = true;
    return &_meta[_metaCount++];
  }
  return nullptr;
}

uint8_t ZynqClass::parsePinNumber(const char* pinStr) {
  if (!pinStr) return 0;
  if (pinStr[0] == 'v' || pinStr[0] == 'V') {
    return (uint8_t)atoi(pinStr + 1);
  }
  return (uint8_t)atoi(pinStr);
}

void ZynqClass::setLabel(uint8_t pin, const char* label) {
  ZynqPinMeta* m = findOrCreateMeta(pin);
  if (m) {
    m->label = label ? label : "";
    _vpinsSynced = false;
  }
}

void ZynqClass::setLabel(const char* pinStr, const char* label) {
  setLabel(parsePinNumber(pinStr), label);
}

bool ZynqClass::send(uint8_t pin, double value) {
  size_t prevCount = _metaCount;
  ZynqPinMeta* m = findOrCreateMeta(pin);
  if (m && m->label.length() == 0) {
    m->label = "Sensor " + String(pin);
    m->type = "float";
    m->readOnly = true;
  }
  if (_metaCount > prevCount) {
    _vpinsSynced = false;
  }

  // Jika sedang berada di dalam Zynq.every(), batching payload
  if (_isBatching) {
    _batchBuffer += ",\"v" + String(pin) + "\":" + String(value, 3);
    _batchItemCount++;
    return true;
  }

  if (!isConnected()) return false;

  uint64_t ts = currentTimestamp();
  String payload = "{\"ts\":" + String(ts) + ",\"seq\":" + String(++_seq) + ",\"v" + String(pin) + "\":" + String(value, 3) + "}";
  return _mqtt.publish(_topicData.c_str(), payload.c_str());
}

bool ZynqClass::send(uint8_t pin, bool value) {
  size_t prevCount = _metaCount;
  ZynqPinMeta* m = findOrCreateMeta(pin);
  if (m && m->label.length() == 0) {
    m->label = "Pin " + String(pin);
    m->type = "bool";
  }
  if (_metaCount > prevCount) {
    _vpinsSynced = false;
  }

  if (_isBatching) {
    _batchBuffer += ",\"v" + String(pin) + "\":" + (value ? "true" : "false");
    _batchItemCount++;
    return true;
  }

  if (!isConnected()) return false;

  uint64_t ts = currentTimestamp();
  String payload = "{\"ts\":" + String(ts) + ",\"seq\":" + String(++_seq) + ",\"v" + String(pin) + "\":" + (value ? "true" : "false") + "}";
  return _mqtt.publish(_topicData.c_str(), payload.c_str());
}

bool ZynqClass::send(const char* pinStr, double value) {
  return send(parsePinNumber(pinStr), value);
}

bool ZynqClass::send(const char* pinStr, bool value) {
  return send(parsePinNumber(pinStr), value);
}

void ZynqClass::syncVpins() {
  if (!isConnected() || _metaCount == 0) return;

  uint64_t ts = currentTimestamp();
  String payload = "{\"ts\":" + String(ts) + ",\"seq\":0,\"vpins\":[";

  for (size_t i = 0; i < _metaCount; i++) {
    if (i > 0) payload += ",";
    String label = _meta[i].label.length() > 0 ? _meta[i].label : (_meta[i].readOnly ? "Sensor " + String(_meta[i].pin) : "Kontrol " + String(_meta[i].pin));
    payload += "{\"vpin\":\"v" + String(_meta[i].pin) + "\",";
    payload += "\"label\":\"" + label + "\",";
    payload += "\"unit\":\"\",";
    payload += "\"type\":\"" + _meta[i].type + "\",";
    payload += "\"readOnly\":" + String(_meta[i].readOnly ? "true" : "false") + "}";
  }
  payload += "]}";

  if (_mqtt.publish(_topicData.c_str(), payload.c_str())) {
    _vpinsSynced = true;
    Serial.println("[Zynq] VPin terdeteksi otomatis dan disinkronkan ke Zynq Cloud.");
  }
}

void ZynqClass::run() {
  ensureWifi();

  if (WiFi.status() == WL_CONNECTED) {
    if (!_mqtt.connected()) {
      unsigned long now = millis();
      if (now - _lastReconnectAttempt > 5000) {
        _lastReconnectAttempt = now;
        if (ensureMqtt()) {
          _lastReconnectAttempt = 0;
        }
      }
    } else {
      _mqtt.loop();

      if (!_vpinsSynced && _metaCount > 0) {
        syncVpins();
      }

      // Jalankan seluruh tugas berkala yang didaftarkan lewat Zynq.every(...)
      unsigned long now = millis();
      for (size_t i = 0; i < _taskCount; i++) {
        if (_tasks[i].enabled && (now - _tasks[i].lastRun >= _tasks[i].interval)) {
          _tasks[i].lastRun = now;
          if (_tasks[i].callback) {
            _isBatching = true;
            _batchBuffer = "";
            _batchItemCount = 0;

            _tasks[i].callback();

            _isBatching = false;
            // Kirim paket batch JSON hemat transmisi
            if (_batchItemCount > 0) {
              uint64_t ts = currentTimestamp();
              String payload = "{\"ts\":" + String(ts) + ",\"seq\":" + String(++_seq) + _batchBuffer + "}";
              _mqtt.publish(_topicData.c_str(), payload.c_str());
            }
          }
        }
      }
    }
  }
}

bool ZynqClass::isConnected() {
  return WiFi.status() == WL_CONNECTED && _mqtt.connected();
}

void ZynqClass::ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 3000) return;
  lastCheck = millis();

  if (WiFi.status() != WL_CONNECTED && _ssid) {
    Serial.print("[Zynq] Menyambungkan WiFi: ");
    Serial.println(_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _wifiPassword);
  }
}

bool ZynqClass::ensureMqtt() {
  if (_projectId.length() == 0 || _deviceId.length() == 0) return false;

  Serial.print("[Zynq] Menyambungkan ke broker Zynq...");

  const char* willTopic = _topicStatus.c_str();
  const char* willPayload = "{\"online\":false}";
  uint8_t willQos = 1;
  bool willRetain = true;

  bool connected = _mqtt.connect(
    _deviceId.c_str(),
    _deviceId.c_str(),
    _deviceToken.c_str(),
    willTopic,
    willQos,
    willRetain,
    willPayload
  );

  if (connected) {
    Serial.println(" Terhubung!");

    _mqtt.publish(_topicStatus.c_str(), "{\"online\":true}", true);
    _mqtt.subscribe(_topicCmd.c_str(), 1);

    syncVpins();
    return true;
  } else {
    Serial.print(" Gagal, rc=");
    Serial.println(_mqtt.state());
    return false;
  }
}

uint64_t ZynqClass::currentTimestamp() {
  time_t now = time(nullptr);
  if (now > 1000000000) {
    return (uint64_t)now * 1000ULL;
  }
  return (uint64_t)millis();
}

void ZynqClass::staticCallback(char* topic, byte* payload, unsigned int length) {
  if (_instance) {
    _instance->handleMessage(topic, payload, length);
  }
}

static String extractJsonString(const String& json, const char* key) {
  int k = json.indexOf(key);
  if (k == -1) return "";
  int colon = json.indexOf(':', k);
  if (colon == -1) return "";
  int firstQuote = json.indexOf('"', colon);
  if (firstQuote == -1) return "";
  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote == -1) return "";
  return json.substring(firstQuote + 1, secondQuote);
}

static String extractJsonValue(const String& json, const char* key) {
  int k = json.indexOf(key);
  if (k == -1) return "";
  int colon = json.indexOf(':', k);
  if (colon == -1) return "";
  int start = colon + 1;
  while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
  int end = start;
  while (end < (int)json.length() && json[end] != ',' && json[end] != '}' && json[end] != ' ' && json[end] != '\r' && json[end] != '\n') end++;
  String val = json.substring(start, end);
  val.trim();
  return val;
}

void ZynqClass::handleMessage(char* topic, byte* payload, unsigned int length) {
  char buffer[length + 1];
  memcpy(buffer, payload, length);
  buffer[length] = '\0';

  String msg = String(buffer);

  // Parse Command ID, VPin, dan Value secara aman terhadap berbagai spasi JSON
  String cmdId = extractJsonString(msg, "\"id\"");
  String pinStr = extractJsonString(msg, "\"vpin\"");
  String valStr = extractJsonValue(msg, "\"val\"");

  uint8_t pinNum = (pinStr.length() > 0) ? parsePinNumber(pinStr.c_str()) : 255;

  ZynqParam param;
  if (valStr.length() > 0) {
    if (valStr.equalsIgnoreCase("true")) {
      param.boolVal = true;
      param.numVal = 1.0;
      param.isBoolean = true;
    } else if (valStr.equalsIgnoreCase("false")) {
      param.boolVal = false;
      param.numVal = 0.0;
      param.isBoolean = true;
    } else {
      param.numVal = valStr.toDouble();
      param.boolVal = (param.numVal != 0.0);
      param.isBoolean = false;
    }
  }

  if (pinNum < 64) {
    // Eksekusi callback yang didaftarkan lewat Zynq.on(pinNum, ...)
    for (size_t i = 0; i < _actionCount; i++) {
      if (_actions[i].pin == pinNum) {
        if (_actions[i].type == ZynqActionEntry::TYPE_INT && _actions[i].intFn) {
          _actions[i].intFn(param.asInt());
        } else if (_actions[i].type == ZynqActionEntry::TYPE_FLOAT && _actions[i].floatFn) {
          _actions[i].floatFn(param.asFloat());
        } else if (_actions[i].type == ZynqActionEntry::TYPE_PARAM && _actions[i].paramFn) {
          _actions[i].paramFn(param);
        }
        break;
      }
    }

    // Balas ACK konfirmasi ke Dashboard Zynq
    if (cmdId.length() > 0) {
      String ackPayload = "{\"id\":\"" + cmdId + "\",\"ok\":true}";
      _mqtt.publish(_topicAck.c_str(), ackPayload.c_str());
    }

    // Sync balik status terkini
    if (param.isBoolean) {
      send(pinNum, param.boolVal);
    } else {
      send(pinNum, param.numVal);
    }
  }
}
