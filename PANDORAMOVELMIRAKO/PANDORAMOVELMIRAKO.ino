#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include "HardwareSerial.h"
#include <Preferences.h>

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



// ===== Temporização =====
unsigned long lastAction    = 0;
const unsigned long interval = 2000; // 5s

// ===== Estados lidos da Serial do UNO =====
float  dist1  = -1.0;  // última leitura válida
float  dist2  = -1.0;  // última leitura válida
float  dist3  = -1.0;  // última leitura válida
float  dist4  = -1.0;  // última leitura válida
String pendingAlert  = "";    // "TIMEOUT" ou "SAFETY"
bool   stopSent      = false; // para só enviar uma vez o stop ao servidor

String formattedDateTime;

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

  setupWEB();
  setupTIME();

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





   setupTELE();
 


}

void loop() {
  // 1) Parseia tudo da UART2 do UNO
  parseUnoSerial();
  loopWEB();
  loopTIME();
  
  // 2) A cada 5s, envia dados
  if (millis() - lastAction >= interval) {
    lastAction = millis();
    parseUnoSerial();
    loopWEB();
    loopTIME();

  }
}
