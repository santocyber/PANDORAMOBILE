// na área global:
WiFiClient clientCam;
HTTPClient   httpCam;

WiFiClient clientSensor;
HTTPClient   httpSensor;

WiFiClient clientcmd;
HTTPClient   httpcmd;



const String urlImage  = "http://sophia.mirako.org/pandora/upload.php";
const String urlSensor  = "http://sophia.mirako.org/pandora/sensor.php";


void setupTELE(){

    // 3) Abre a conexão HTTP UMA vez só, e marca para reaproveitar
  httpCam.begin(clientCam, urlImage);
  httpCam.setReuse(true);
  httpCam.addHeader("Content-Type", "image/jpeg");

  httpSensor.begin(clientSensor, urlSensor);
  httpSensor.setReuse(true);
  httpSensor.addHeader("Content-Type", "application/json");

     // Tarefa de streaming contínuo
  xTaskCreatePinnedToCore(
    cameraStreamTask,
    "camTask",
    10 * 1024,
    nullptr,
    1,
    &cameraTaskHandle,
    0
  );
  
  }

// === TASK de streaming MJPEG (1 fps) ===
void cameraStreamTask(void *parameter) {
  for (;;) {

    sendCameraImage();
    sendSensorAndGetCommand();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// === Envia snapshot JPEG único via POST ===
void sendCameraImage() {
        Serial.println("FUNCAO IMAGEM...");
   camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Erro ao capturar frame");
    vTaskDelay(pdMS_TO_TICKS(500));
    return;
  }

  // envia sem recriar a conexão
  int code = httpCam.sendRequest("POST", fb->buf, fb->len);
  Serial.printf("HTTP code: %d\n", code);

  if (code > 0) {
    String body = httpCam.getString();
    Serial.print("Resposta: ");
    Serial.println(body);
  } else {
    Serial.printf("Falha: %s\n", httpCam.errorToString(code).c_str());
  }

  // devolve o frame
  esp_camera_fb_return(fb);
  Serial.println("FUNCAO IMAGEM FIM!");

}


void sendSensorAndGetCommand() {

          Serial.println("FUNCAO SENSOR...");

  // 1) Monta JSON com as 4 distâncias, timestamp e alert
  StaticJsonDocument<256> doc;
  doc["distancia1"] = dist1;
  doc["distancia2"] = dist2;
  doc["distancia3"] = dist3;
  doc["distancia4"] = dist4;
  doc["timestamp"]  = formattedDateTime;      // ou sua string formatada
  if (pendingAlert.length()) {
    doc["alert"] = pendingAlert;
  }
  String body;
  serializeJson(doc, body);
 
  int code = httpSensor.POST(body);

  if (code == 200) {
    String cmd = httpSensor.getString();
    cmd.trim();
    Serial.printf("Novo comando: %s\n", cmd.c_str());
    if (cmd.length()) {
      executarComando(cmd);
    }
  } else {
    Serial.printf("Erro POST sensor: %d\n", code);
  }

  // 3) Limpa flag de alerta para a próxima iteração
  pendingAlert = "";
          Serial.println("FUNCAO SENSOR END!");

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
      dist1 = num.toFloat();
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
            Serial.println("FUNCAO SEND STOP!");

  httpcmd.begin(clientcmd, "http://sophia.mirako.org/pandora/comando.php?cmd=stop");
  httpcmd.GET();
  httpcmd.end();
            Serial.println("FUNCAO SENDSTOP END!");

}
