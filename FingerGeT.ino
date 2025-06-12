#include "thingProperties.h"
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

#define IN1 22
#define IN2 23

// Sensor nos pinos RX=16, TX=17
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger(&mySerial);

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Garante que o motor esteja desligado no início
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  // Conectar à Nuvem Arduino IoT
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Inicializa o sensor de digital
  Serial.println("🔍 Inicializando o sensor de impressão digital...");
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);
  delay(100);

  if (finger.verifyPassword()) {
    Serial.println("✅ Sensor conectado com sucesso!");
  } else {
    Serial.println("❌ Sensor não detectado. Verifique a fiação.");
    while (true) delay(1); // Para tudo
  }

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.println("⚠️ Nenhuma digital cadastrada.");
  } else {
    Serial.printf("📦 %d digitais armazenadas na memória.\n", finger.templateCount);
  }
}

void loop() {
  ArduinoCloud.update(); // Mantém a conexão com a nuvem

  getFingerprintID(); // Tenta reconhecer uma digital
  delay(1500);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.printf("🆔 ID %d reconhecido! Confiança: %d\n", finger.fingerID, finger.confidence);
    accessLog = "👤 Pessoa com ID #" + String(finger.fingerID) + " entrou";
    Serial.println("🌐 Log atualizado na nuvem: " + accessLog);

    // Liga o motor
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    delay(3000);

    // Desliga o motor
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("❌ Nenhuma correspondência encontrada.");
  } else {
    Serial.println("❌ Erro na busca pela digital.");
  }

  return p;
}

void onAccessLogChange() {
  // Opcional: executa quando accessLog é alterado pela nuvem
  Serial.println("📝 accessLog alterado remotamente: " + accessLog);
}
