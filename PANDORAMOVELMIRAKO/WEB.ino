#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

WebServer server(80);
Preferences preferences;


const char* AP_SSID = "PandoraAP";
const char* AP_PASS = "12345678";
const char* SSIDCONFIG = "InternetSA";
const char* PASSCONFIG = "cadebabaca";
bool apModeActive = false;
String macAddress;

unsigned long apModeStartTime = 0;
const unsigned long AP_TIMEOUT = 120000; // 3 minutos
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL = 30000; // 30 segundos


String networksHTML = "";

String getPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: #f0f0f0;
    }
    .btn {
      display: block;
      width: 100%;
      padding: 20px;
      margin: 10px 0;
      font-size: 24px;
      border: none;
      border-radius: 10px;
      cursor: pointer;
    }
    .network-btn {
      background: #4CAF50;
      color: white;
    }
    .action-btn {
      background: #2196F3;
      color: white;
    }
    .danger-btn {
      background: #f44336;
      color: white;
    }
    input {
      width: 100%;
      padding: 20px;
      margin: 10px 0;
      font-size: 24px;
      border: 2px solid #ddd;
      border-radius: 10px;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
    }
  </style>
  <script>
    function loadNetworks() {
      fetch('/scan')
        .then(response => response.json())
        .then(data => {
          const container = document.getElementById('networks');
          container.innerHTML = '';
          data.forEach(network => {
            const btn = document.createElement('button');
            btn.className = 'btn network-btn';
            btn.innerHTML = `${network.ssid} (${network.rssi} dBm)`;
            btn.onclick = () => {
              document.getElementById('ssid').value = network.ssid;
            };
            container.appendChild(btn);
          });
        });
    }

    function rescan() {
      document.getElementById('networks').innerHTML = 'Escaneando...';
      loadNetworks();
    }

    window.onload = loadNetworks;
  </script>
</head>
<body>
  <div class="container">
    <h1 style="font-size: 36px; text-align: center;">Configurar Wi-Fi</h1>
    
    <div id="networks"></div>

    <input type="text" id="ssid" name="ssid" placeholder="Nome da Rede" readonly>

    <input type="password" id="password" name="password" placeholder="Senha">

    <form action="/save" method="POST">
      <input type="hidden" name="ssid" id="formSsid">
      <input type="hidden" name="password" id="formPassword">
      <button type="submit" class="btn action-btn" 
              onclick="document.getElementById('formSsid').value = document.getElementById('ssid').value;
                       document.getElementById('formPassword').value = document.getElementById('password').value;">
        SALVAR CONFIGS
      </button>
    </form>

    <button class="btn danger-btn" onclick="location.href='/delete'">
      EXCLUIR CONFIGS
    </button>

    <button class="btn action-btn" onclick="rescan()" style="font-size: 18px; padding: 10px;">
      ATUALIZAR REDES
    </button>
  </div>
</body>
</html>
)rawliteral";
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; ++i) {
    if (i) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i));
    json += "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  
  if(ssid.length() > 0 && pass.length() > 0){
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    server.send(200, "text/html", "<h1 style='font-size:48px;text-align:center;'>Configs salvas! Reiniciando...</h1>");
    delay(1000);
    ESP.restart();
  }
}

void handleDelete() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    Serial.println("Wi-Fi apagado!");
  server.send(200, "text/html", "<h1 style='font-size:48px;text-align:center;'>Configs excluidas! Reiniciando...</h1>");
  delay(1000);
  ESP.restart();
}




bool connectToWiFi() {
  // 1. Tentar rede pré-configurada
  Serial.println("\nTentando rede pré-configurada...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSIDCONFIG, PASSCONFIG);

  unsigned long start = millis();
  while(WiFi.status() != WL_CONNECTED && (millis() - start) < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado à rede pré-configurada!");
    macAddress = WiFi.macAddress();
    return true;
  }

  // 2. Tentar credenciais salvas
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  
  if(ssid.length() > 0) {
    Serial.println("\nTentando credenciais salvas...");
    WiFi.disconnect(true);
    delay(1000);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    start = millis();
    while(WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
      delay(500);
      Serial.print(".");
    }
    
    if(WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConectado usando credenciais salvas!");
      macAddress = WiFi.macAddress();

      return true;
    }
  }
  return false;
}

void setupWEB() {
  preferences.begin("wifi", false);

  if(connectToWiFi()) {
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return;
  }

  // Se falhar, iniciar AP
  Serial.println("Iniciando modo AP...");
  apModeActive = true;
  apModeStartTime = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  server.on("/", []() { server.send(200, "text/html", getPage()); });
  server.on("/scan", handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/delete", handleDelete);
  server.begin();
  
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loopWEB() {
  if(apModeActive) {
    server.handleClient();

    // Verificar timeout de 3 minutos
    if(millis() - apModeStartTime >= AP_TIMEOUT) {
      Serial.println("\nTempo limite do AP atingido. Desligando modo AP...");
      
      // Desligar AP e servidor
      apModeActive = false;
      WiFi.softAPdisconnect(true);
      server.close();
      
      // Forçar nova tentativa de conexão
      lastReconnectAttempt = 0; // Reiniciar contador
    }
  }
  else {
    // Tentar reconexão periódica
    if(WiFi.status() != WL_CONNECTED && (millis() - lastReconnectAttempt >= RECONNECT_INTERVAL)) {
      Serial.println("\nIniciando tentativa de reconexão...");
      lastReconnectAttempt = millis();
      
      if(connectToWiFi()) {
        Serial.println("Conectado com sucesso!");
      } else {
        Serial.println("Falha na reconexão. Próxima tentativa em 30 segundos.");
      }
    }
  }
  delay(1);
}
