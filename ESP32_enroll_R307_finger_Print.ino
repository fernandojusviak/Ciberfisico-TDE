#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

// Usa UART2 (GPIO16 = RX2, GPIO17 = TX2 no ESP32)
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger(&mySerial);

uint8_t id;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🔐 Inicializando sensor de impressão digital...");

  mySerial.begin(57600, SERIAL_8N1, 16, 17); // GPIO16 = RX, GPIO17 = TX
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Sensor de impressão digital detectado!");
  } else {
    Serial.println("❌ Sensor não detectado. Verifique a conexão!");
    while (true) delay(1);
  }

  Serial.println("📋 Parâmetros do sensor:");
  finger.getParameters();
  Serial.printf("Status: 0x%X\n", finger.status_reg);
  Serial.printf("Sys ID: 0x%X\n", finger.system_id);
  Serial.printf("Capacidade: %d\n", finger.capacity);
  Serial.printf("Nível de segurança: %d\n", finger.security_level);
  Serial.printf("Endereço do dispositivo: 0x%X\n", finger.device_addr);
  Serial.printf("Tamanho do pacote: %d\n", finger.packet_len);
  Serial.printf("Baud rate: %d\n", finger.baud_rate);
}

uint8_t readNumber() {
  while (!Serial.available());
  String input = Serial.readStringUntil('\n');
  input.trim();  // remove espaços ou quebras de linha extras
  return (uint8_t)input.toInt();
}


void loop() {
  Serial.println("\n🖐️ Opções:");
  Serial.println("1. Cadastrar digital");
  Serial.println("2. Verificar digital");
  Serial.println("3. Deletar digital");
  Serial.println("Digite uma opção:");

  int opcao = readNumber();

  if (opcao == 1) {
    Serial.println("Digite o ID da digital (1 a 127):");
    id = readNumber();
    if (id == 0 || id > 127) {
      Serial.println("❌ ID inválido!");
      return;
    }
    Serial.printf("➡️ Cadastrando ID #%d\n", id);
    while (!enrollFingerprint());
    delay(2000);
  } 
  else if (opcao == 2) {
    verificarDigital();
    delay(2000);
  } 
  else if (opcao == 3) {
    Serial.println("🗑️ Qual ID deseja deletar?");
    uint8_t deleteId = readNumber();
    if (deleteId > 0 && deleteId <= 127) {
      deleteFingerprint(deleteId);
    } else {
      Serial.println("❌ ID inválido para deletar.");
    }
    delay(2000);
  } 
  else {
    Serial.println("❌ Opção inválida.");
  }
}
bool deleteFingerprint(uint8_t id) {
  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.printf("🗑️ Digital com ID #%d deletada com sucesso!\n", id);
    return true;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("❌ Erro de comunicação ao tentar deletar.");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("❌ ID inválido para deletar.");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("❌ Erro ao apagar da memória flash.");
  } else {
    Serial.println("❌ Erro desconhecido ao deletar.");
  }
  return false;
}


void verificarDigital() {
  Serial.println("👉 Coloque o dedo para verificação...");

  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz() != FINGERPRINT_OK) {
    Serial.println("❌ Erro ao converter imagem.");
    return;
  }

  if (finger.fingerSearch() == FINGERPRINT_OK) {
    Serial.printf("✅ Digital reconhecida! ID: #%d\n", finger.fingerID);
  } else {
    Serial.println("❌ Digital não reconhecida.");
  }
}

bool enrollFingerprint() {
  int p = -1;

  unsigned long tempoInicio = millis(); // 🕒 Início da medição

  Serial.println("👉 Coloque o dedo no sensor...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) Serial.print(".");
    else if (p == FINGERPRINT_OK) Serial.println("\n📸 Imagem capturada.");
    else return reportError(p);
  }

  if (!convertImage(1)) return false;

  Serial.println("✋ Remova o dedo...");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("👉 Coloque o mesmo dedo novamente...");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) Serial.print(".");
    else if (p == FINGERPRINT_OK) Serial.println("\n📸 Imagem capturada.");
    else return reportError(p);
  }

  if (!convertImage(2)) return false;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) return reportError(p);

  Serial.println("✅ Impressões combinadas. Armazenando...");

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("💾 Digital armazenada com sucesso!");

    unsigned long tempoFim = millis(); // 🕒 Fim da medição

    // 📊 Relatório de desempenho
    Serial.println("\n📊 MÉTRICAS DE DESEMPENHO:");
    Serial.print("⏱️ Duração do processo de cadastro: ");
    Serial.print(tempoFim - tempoInicio);
    Serial.println(" ms");

    // 🔋 Estimativa de consumo energético (exemplo teórico)
    float correnteSensor = 0.05; // 50 mA = sensor + ESP32 sem motor
    float tensao = 3.3;          // Tensão do ESP32 ou sensor
    float tempoSegundos = (tempoFim - tempoInicio) / 1000.0;
    float energiaConsumida = correnteSensor * tensao * tempoSegundos;

    Serial.print("🔋 Energia estimada consumida: ");
    Serial.print(energiaConsumida, 4);
    Serial.println(" joules");

    return true;
  } else {
    return reportError(p);
  }
}

bool convertImage(uint8_t slot) {
  int p = finger.image2Tz(slot);
  switch (p) {
    case FINGERPRINT_OK: Serial.println("🔄 Imagem convertida."); return true;
    case FINGERPRINT_IMAGEMESS: Serial.println("❌ Imagem com ruído."); break;
    case FINGERPRINT_FEATUREFAIL: case FINGERPRINT_INVALIDIMAGE:
      Serial.println("❌ Não foi possível extrair as características.");
      break;
    default: reportError(p);
  }
  return false;
}

bool reportError(int code) {
  switch (code) {
    case FINGERPRINT_PACKETRECIEVEERR: Serial.println("❌ Erro de comunicação."); break;
    case FINGERPRINT_ENROLLMISMATCH: Serial.println("❌ As digitais não coincidem."); break;
    case FINGERPRINT_BADLOCATION: Serial.println("❌ Local inválido para armazenamento."); break;
    case FINGERPRINT_FLASHERR: Serial.println("❌ Erro ao gravar na memória flash."); break;
    default: Serial.println("❌ Erro desconhecido."); break;
  }
  return false;
}
