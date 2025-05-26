#include <WiFi.h>
#include <time.h>


// Servidor NTP e offsets de fuso
const char*  ntpServer         = "pool.ntp.org";
const long   gmtOffset_sec     = -3 * 3600;  // UTC-3 :contentReference[oaicite:3]{index=3}
const int    daylightOffset_sec= 0;

// String para exibir a data/hora formatada

// Inicializa o SNTP e aguarda sincronização
void setupTIME() {
  // Configura fuso horário e servidor NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);   // chama sntp_init :contentReference[oaicite:4]{index=4}

  Serial.print("Sincronizando tempo");
  struct tm timeinfo;
  int retry = 0;
  const int retryCount = 2;

  // Aguarda até getLocalTime obter sucesso ou esgotar tentativas
  while (!getLocalTime(&timeinfo) && ++retry <= retryCount) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  if (retry > retryCount) {
    Serial.println("Falha ao sincronizar o tempo");            // timeout sync :contentReference[oaicite:5]{index=5}
  } else {
    Serial.println("Tempo sincronizado com sucesso!");         // sync OK
  }
}

// Atualiza 'formattedDateTime' usando a hora do RTC interno
void updateDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter o tempo");                  // getLocalTime falhou :contentReference[oaicite:6]{index=6}
    formattedDateTime = "";
    return;
  }

  // Formata em DD/MM/YYYY HH:MM:SS
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  formattedDateTime = String(buffer);
}

// Loop de atualização a cada 5 segundos
void loopTIME() {
 
  
  
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 5000) {
    updateDateTime();
    // Exemplo de exibição no Serial (ou OLED)
    Serial.println(formattedDateTime);
    lastUpdate = millis();
    
  }
}
