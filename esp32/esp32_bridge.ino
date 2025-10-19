#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ====== CONFIG ======
const char* WIFI_SSID     = "TU_SSID";                         // ← tu Wi-Fi
const char* WIFI_PASSWORD = "TU_PASS";                         // ← tu clave
const char* YOUR_HOST     = "https://ministacionamiento.onrender.com"; // ← tu Render
const char* API_TOKEN     = "1ba48e38c80522fc03e1a045241fab03";        // ← igual al de Render
const char* SLOT_ID       = "S1";                               // este ESP32 reporta el puesto S1
// =====================

// ESP32-S3: ajusta según tu placa
#define RXD2 41
#define TXD2 42
HardwareSerial& ARD = Serial2;

int lastProx = 0;       // 0 libre, 1 ocupado
int currentMotor = -1;  // -1 desconocido
unsigned long tPoll=0, tTel=0;

// TLS: para demo usamos setInsecure(). Para producción, usa setCACert con ISRG Root X1.
// const char* ISRG_ROOT_X1 = R"(-----BEGIN CERTIFICATE-----
// ... CA de Let's Encrypt (ISRG Root X1) ...
// -----END CERTIFICATE-----)";

bool httpGet(const String& url, String& payload){
  WiFiClientSecure client;
  client.setInsecure();                 // DEMO. En prod: client.setCACert(ISRG_ROOT_X1);
  HTTPClient http;
  if(!http.begin(client, url)) return false;
  http.addHeader("X-API-Key", API_TOKEN);  // header para GET también
  int code = http.GET();
  if(code > 0) payload = http.getString();
  http.end();
  return code == 200;
}

bool httpPostJson(const String& url, const String& json){
  WiFiClientSecure client;
  client.setInsecure();                 // DEMO. En prod: client.setCACert(ISRG_ROOT_X1);
  HTTPClient http;
  if(!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_TOKEN);
  int code = http.POST(json);
  http.end();
  return code == 200;
}

void parseLine(const String& line){
  // Arduino envía "PROX:0" o "PROX:1"
  if(line.startsWith("PROX:")){
    int v = line.substring(5).toInt();
    lastProx = (v != 0) ? 1 : 0;
  }
}

void setup(){
  Serial.begin(115200);
  ARD.begin(9600, SERIAL_8N1, RXD2, TXD2);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando");
  int tries=0; 
  while (WiFi.status()!=WL_CONNECTED && tries<60){ delay(250); Serial.print("."); tries++; }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

void loop(){
  // UART no bloqueante desde Arduino
  while(ARD.available()){
    String line = ARD.readStringUntil('\n'); line.trim();
    if(line.length()) parseLine(line);
  }

  // 1) Consultar comando deseado cada 1 s
  if(millis() - tPoll > 1000){
    tPoll = millis();
    if(WiFi.status() == WL_CONNECTED){
      String body;
      if(httpGet(String(YOUR_HOST) + "/api/cmd", body)){
        // body esperado: {"motor_desired":"on"} o "off"
        int i = body.indexOf("\"motor_desired\":\"");
        if(i >= 0){
          int j = body.indexOf('"', i + 17);
          String val = body.substring(i + 17, j); // on|off
          int desired = (val == "on") ? 1 : 0;
          if(desired != currentMotor){
            ARD.println(desired ? "MOTOR:ON" : "MOTOR:OFF");
            currentMotor = desired;
            Serial.printf("CMD -> Arduino: %s\n", desired ? "ON" : "OFF");
          }
        }
      }
    }
  }

  // 2) Enviar telemetría cada 1 s (con SLOT)
  if(millis() - tTel > 1000){
    tTel = millis();
    if(WiFi.status() == WL_CONNECTED){
      // Enviamos también el slot para el “mapa de estacionamiento”
      String json = String("{\"prox\":") + lastProx + ",\"slot\":\"" + SLOT_ID + "\"}";
      httpPostJson(String(YOUR_HOST) + "/api/telemetry", json);
    }
  }
}
