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
  getFingerprintID();    // Chama a função corretamente
  delay(1500);           // Pequeno atraso para nova leitura
}

// Essa parte estava dentro do loop() por engano
uint8_t getFingerprintID() {
  unsigned long tempoInicio = millis();  // ⏱️ Início

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;

  p = finger.fingerFastSearch();
  unsigned long tempoReconhecimento = millis();

  if (p == FINGERPRINT_OK) {
    Serial.printf("🆔 ID %d reconhecido! Confiança: %d\n", finger.fingerID, finger.confidence);

    accessLog = "👤 Pessoa com ID #" + String(finger.fingerID) + " entrou";
    ArduinoCloud.update(); // força envio imediato
    unsigned long tempoNuvem = millis();

    // Relatório de latência
    Serial.println("📊 MÉTRICAS DE DESEMPENHO:");
    Serial.print("⏱️ Tempo de reconhecimento: ");
    Serial.print(tempoReconhecimento - tempoInicio);
    Serial.println(" ms");

    Serial.print("☁️ Tempo até atualização na nuvem: ");
    Serial.print(tempoNuvem - tempoReconhecimento);
    Serial.println(" ms");

    // Simula o motor girando por 3 segundos
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    delay(3000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    unsigned long tempoFim = millis();
    Serial.print("⚙️ Duração total do processo: ");
    Serial.print(tempoFim - tempoInicio);
    Serial.println(" ms");

    // Consumo estimado (exemplo)
    float correnteMotor = 0.3; // em amperes (300mA típico de motores pequenos)
    float tensao = 5.0;        // alimentação do motor
    float tempoSegundos = (tempoFim - tempoInicio) / 1000.0;
    float energiaConsumida = correnteMotor * tensao * tempoSegundos;

    Serial.print("🔋 Energia estimada consumida (motor): ");
    Serial.print(energiaConsumida, 3);
    Serial.println(" joules");
  }
  else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("❌ Nenhuma correspondência encontrada.");
  }
  else {
    Serial.println("❌ Erro na busca pela digital.");
  }

  return p;
}

void onAccessLogChange() {
  // Opcional: executa quando accessLog é alterado pela nuvem
  Serial.println("📝 accessLog alterado remotamente: " + accessLog);
}
