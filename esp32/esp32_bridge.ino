#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

const char* WIFI_SSID     = "TU_SSID";
const char* WIFI_PASSWORD = "TU_PASS";
const char* YOUR_HOST     = "https://tu-app.onrender.com"; // sin '/' final

#define RXD2 41
#define TXD2 42
HardwareSerial& ARD = Serial2;

int lastProx = 0;         // 0 libre, 1 ocupado
int currentMotor = -1;    // -1 desconocido
unsigned long tPoll=0, tTel=0;

void setup(){
  Serial.begin(115200);
  ARD.begin(9600, SERIAL_8N1, RXD2, TXD2);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando");
  int tries=0; while (WiFi.status()!=WL_CONNECTED && tries<60){ delay(250); Serial.print("."); tries++; }
  Serial.println(); Serial.print("IP: "); Serial.println(WiFi.localIP());
}

bool httpGet(const String& url, String& payload){
  WiFiClientSecure client; client.setInsecure(); // demo: sin validar CA
  HTTPClient http; if(!http.begin(client, url)) return false;
  int code = http.GET(); if(code>0) payload=http.getString(); http.end(); return code==200;
}

bool httpPostJson(const String& url, const String& json){
  WiFiClientSecure client; client.setInsecure(); // demo
  HTTPClient http; if(!http.begin(client, url)) return false;
  http.addHeader("Content-Type","application/json");
  int code = http.POST(json); http.end(); return code==200;
}

void parseLine(const String& line){
  if(line.startsWith("PROX:")){
    int v = line.substring(5).toInt();
    lastProx = (v!=0) ? 1 : 0;
  }
}

void loop(){
  // UART no bloqueante
  while(ARD.available()){
    String line = ARD.readStringUntil('\n'); line.trim();
    if(line.length()) parseLine(line);
  }

  // 1) Consultar comando cada 1 s
  if(millis()-tPoll>1000){
    tPoll=millis();
    if(WiFi.status()==WL_CONNECTED){
      String body;
      if(httpGet(String(YOUR_HOST)+"/api/cmd", body)){
        int i = body.indexOf("\"motor_desired\":\"");
        if(i>=0){
          int j = body.indexOf('"', i+17);
          String val = body.substring(i+17, j); // on|off
          int desired = (val=="on") ? 1 : 0;
          if(desired != currentMotor){
            ARD.println(desired ? "MOTOR:ON" : "MOTOR:OFF");
            currentMotor = desired;
            Serial.printf("CMD -> Arduino: %s\n", desired ? "ON" : "OFF");
          }
        }
      }
    }
  }

  // 2) Enviar telemetría cada 1 s
  if(millis()-tTel>1000){
    tTel=millis();
    if(WiFi.status()==WL_CONNECTED){
      String json = String("{\"prox\":")+lastProx+"}";
      httpPostJson(String(YOUR_HOST)+"/api/telemetry", json);
    }
  }
}
