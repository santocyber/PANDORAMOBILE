#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include "HardwareSerial.h"

// ===== UART para o Uno via GPIO12/13 =====
HardwareSerial UnoSerial(2);
static const int RX_PIN = 12;
static const int TX_PIN = 13;

// ===== Flash onboard =====
#define FLASH_GPIO_NUM 4

// ===== Câmera AI-Thinker =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== Wi-Fi e servidor =====
const char* ssid     = "InternetSA";
const char* password = "cadebabaca";
const String server  = "http://sophia.mirako.org/pandora/";

// ===== Temporização =====
unsigned long lastAction    = 0;
const unsigned long interval = 5000; // 5s

// ===== Estados lidos da Serial do UNO =====
float  lastDistance  = -1.0;  // última leitura válida
String pendingAlert  = "";    // "TIMEOUT" ou "SAFETY"
bool   stopSent      = false; // para só enviar uma vez o stop ao servidor

// ===== Protótipos =====
void cameraStreamTask(void*);
void sendCameraImage();
void sendSensorData();
void checkCommand();
void executarComando(const String &cmd);
void parseUnoSerial();
void sendStopToServer();

TaskHandle_t cameraTaskHandle = nullptr;

void setup() {
  Serial.begin(115200);
  UnoSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(FLASH_GPIO_NUM, OUTPUT);
  digitalWrite(FLASH_GPIO_NUM, LOW);

  // Conecta Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi OK");

  // Configura câmera
  camera_config_t cfg;
  cfg.ledc_channel    = LEDC_CHANNEL_0;
  cfg.ledc_timer      = LEDC_TIMER_0;
  cfg.pin_d0          = Y2_GPIO_NUM;
  cfg.pin_d1          = Y3_GPIO_NUM;
  cfg.pin_d2          = Y4_GPIO_NUM;
  cfg.pin_d3          = Y5_GPIO_NUM;
  cfg.pin_d4          = Y6_GPIO_NUM;
  cfg.pin_d5          = Y7_GPIO_NUM;
  cfg.pin_d6          = Y8_GPIO_NUM;
  cfg.pin_d7          = Y9_GPIO_NUM;
  cfg.pin_xclk        = XCLK_GPIO_NUM;
  cfg.pin_pclk        = PCLK_GPIO_NUM;
  cfg.pin_vsync       = VSYNC_GPIO_NUM;
  cfg.pin_href        = HREF_GPIO_NUM;
  cfg.pin_sscb_sda    = SIOD_GPIO_NUM;
  cfg.pin_sscb_scl    = SIOC_GPIO_NUM;
  cfg.pin_pwdn        = PWDN_GPIO_NUM;
  cfg.pin_reset       = RESET_GPIO_NUM;
  cfg.xclk_freq_hz    = 20000000;
  cfg.pixel_format    = PIXFORMAT_JPEG;
  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_QVGA;
    cfg.jpeg_quality = 10;
    cfg.fb_count     = 2;
  } else {
    cfg.frame_size   = FRAMESIZE_CIF;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
  }
  esp_camera_init(&cfg);

  // Tarefa de streaming contínuo
  xTaskCreatePinnedToCore(
    cameraStreamTask,
    "camTask",
    15 * 1024,
    nullptr,
    1,
    &cameraTaskHandle,
    1
  );
}

void loop() {
  // 1) Parseia tudo da UART2 do UNO
  parseUnoSerial();

  // 2) A cada 5s, envia dados
  if (millis() - lastAction >= interval) {
    lastAction = millis();
    sendCameraImage();
    sendSensorData();
    checkCommand();
  }
}

// === TASK de streaming MJPEG (1 fps) ===
void cameraStreamTask(void *parameter) {
  for (;;) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      HTTPClient http;
      http.begin(server + "upload.php");
      http.addHeader("Content-Type", "image/jpeg");
      http.POST(fb->buf, fb->len);
      http.end();
      esp_camera_fb_return(fb);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// === Envia snapshot JPEG único via POST ===
void sendCameraImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;
  HTTPClient http;
  http.begin(server + "upload.php");
  http.addHeader("Content-Type", "image/jpeg");
  http.POST(fb->buf, fb->len);
  http.end();
  esp_camera_fb_return(fb);
}

// === POST JSON com distancia + alert (se houver) ===
void sendSensorData() {
  StaticJsonDocument<256> doc;
  if (lastDistance >= 0) doc["distancia"] = lastDistance;
  else                   doc["distancia"] = nullptr;
  doc["timestamp"] = millis();

  if (pendingAlert != "") {
    doc["alert"] = pendingAlert;
    if (!stopSent) {
      sendStopToServer();
      stopSent = true;
    }
  }

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(server + "sensor.php");
  http.addHeader("Content-Type", "application/json");
  http.POST(payload);
  http.end();

  pendingAlert = "";  // limpa para o próximo ciclo
}

// === GET comando.php e repassa ao UNO ===
void checkCommand() {
  HTTPClient http;
  http.begin(server + "comando.php");
  int code = http.GET();
  if (code == 200) {
    String cmd = http.getString();
    cmd.trim();
    executarComando(cmd);
  }
  http.end();
        sendStopToServer();

}

// === Mapeia texto de comando para UART2 / flash ===
void executarComando(const String &cmdIn) {
  String cmd = cmdIn;
  if (cmd.equalsIgnoreCase("flash_on")) {
    digitalWrite(FLASH_GPIO_NUM, HIGH);
    return;
  }
  if (cmd.equalsIgnoreCase("flash_off")) {
    digitalWrite(FLASH_GPIO_NUM, LOW);
    return;
  }
  if (cmd.startsWith("A")) {
    UnoSerial.println(cmd);
    return;
  }

  char c = cmd.charAt(0);
  switch (c) {
    case 'f': UnoSerial.write('f'); break;
    case 'b': UnoSerial.write('b'); break;
    case 'c': UnoSerial.write('c'); break;
    case 'w': UnoSerial.write('w'); break;
    case 's': UnoSerial.write('s'); break;
    case 'L': UnoSerial.write('L'); break;
    case 'R': UnoSerial.write('R'); break;
    default: return;
  }
  if (c != 's') stopSent = false;  // permite reenviar alertas
}

// === Parse genérico da UART2 do UNO para DIST / TIMEOUT / SAFETY ===
void parseUnoSerial() {
  while (UnoSerial.available()) {
    String line = UnoSerial.readStringUntil('\n');
    line.trim();
Serial.println("SERIAL RECBIDO ");
Serial.println(line);
    // DIST:
    int p = line.indexOf("DIST:");
    if (p >= 0) {
      int cm = line.indexOf("cm", p);
      String num = line.substring(p + 5,
                                 cm > p + 5 ? cm : line.length());
      lastDistance = num.toFloat();
    }
    // TIMEOUT
    if (line.indexOf("TIMEOUT") >= 0) {
      pendingAlert = "TIMEOUT";
    }
    // SAFETY
    if (line.indexOf("SAFETY") >= 0) {
      pendingAlert = "SAFETY";
    }
  }
}

// === Envia GET comando.php?cmd=stop para limpar o comando no servidor ===
void sendStopToServer() {
  HTTPClient http;
  http.begin(server + "comando.php?cmd=stop");
  http.GET();
  http.end();
}
