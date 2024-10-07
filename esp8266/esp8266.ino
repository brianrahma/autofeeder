#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char *dbase = "http://omahiot.net/autofeeder/autofeeder_monitoring.php";  //database
const char* ssid = "TP-Link_D66B";
const char* pass = "WWW.omahiot.NET";

String datetime;
String sensords, sensorph;

unsigned long lastTime = 0;

WiFiClient client;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connectToWifi();
}

void loop() {
  
 String msg = "";
  while (Serial.available() > 0) {
    sensorph = Serial.readStringUntil('@');
    sensords = Serial.readStringUntil('#');
    datetime = Serial.readStringUntil('$');
    
    Serial.println("pemisah");
    if (checkContain(sensorph) && checkContain(sensords) && checkContain(datetime)) {
      Serial.println(sensorph);
      Serial.println(sensords);
      Serial.println(datetime);
      kirimDataKeServer();   
      delay(1000);
    }
  }
}

void kirimDataKeServer() {
  //--------------start to transmit data sensor to server----------//
  if ((WiFi.status() == WL_CONNECTED)) {
    // Buat objek JSON
    StaticJsonDocument<200> doc;

    // Isi data ke objek JSON
    doc["ph"] = sensorph.toFloat();
    doc["ds"] = sensords.toFloat();
    doc["waktu"] = datetime;

    // Buat string JSON dari objek JSON
    String jsonStr;
    serializeJson(doc, jsonStr);

    // Kirim data ke server
    HTTPClient http;
    http.begin(client, dbase);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonStr);

    if (httpResponseCode == 200) {  
      Serial.println("Data is sent successfully");
    } else {
      Serial.print("Error in sending data. Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void connectToWifi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {  //Loop which makes a point every 500ms until the connection process has finished
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP-Address: ");
  Serial.println(WiFi.localIP());
}

String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool checkContain(String text) {
  bool status = true;
  if (text.indexOf("x") > 0 || text.indexOf("!") > 0 || text.indexOf("@") > 0
      || text.indexOf("#") > 0 || text.indexOf("$") > 0 || text.indexOf("%") > 0
      || text.indexOf("&") > 0 || text.indexOf("x") > 0) {
    status = false;
  }
  return status;
}