#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* dbase = "http://omahiot.net/autofeeder/autofeeder_monitoring.php";  //database
const char* ssid = "TP-Link_D66B";
const char* pass = "WWW.omahiot.NET";

String datetime;
String sensords, sensorph;

unsigned long lastTime = 0;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 2000;  // Periksa koneksi setiap 10 detik
// Flag to track WiFi status
bool wifiConnected = false;

WiFiClient client;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  connectToWifi();
}

void loop() {
  // Periksa apakah ada data dari ATmega328P
  String msg = "";
  if (Serial.available() > 0) {
    msg = Serial.readString();
    int atIndex = msg.indexOf('@');
    int hashIndex = msg.indexOf('#');
    int dollarIndex = msg.indexOf('$');

    if (atIndex != -1 && hashIndex != -1 && dollarIndex != -1) {
      sensorph = msg.substring(0, atIndex);
      sensords = msg.substring(atIndex + 1, hashIndex);
      datetime = msg.substring(hashIndex + 1, dollarIndex);

      if (checkContain(sensorph) && checkContain(sensords) && checkContain(datetime)) {
        Serial.println("Data valid, siap dikirim:");
        Serial.println(sensorph);
        Serial.println(sensords);
        Serial.println(datetime);
        kirimDataKeServer();
      }
    }
  }
  // Bagian B: Mengecek status WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("No WiFi");
      wifiConnected = false;
    }
    reconnectWiFi();
  } else {
    if (!wifiConnected) {  // Kirim pesan hanya jika status berubah menjadi terhubung
      Serial.println("Connected");
      wifiConnected = true;
    }
  }
  // Delay utama untuk mengontrol frekuensi loop
  delay(500);  // Waktu lebih singkat agar dapat membaca serial dan WiFi secara bersamaan
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
      Serial.print("Data is sent succesfully.");  // Kirim status OK ke ATmega328P
    } else {
      Serial.print("Error in sending data. code: ");  // Kirim pesan error ke ATmega328P
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void connectToWifi() {
  WiFi.begin(ssid, pass);
  unsigned long startTime = millis();      // Mulai waktu percobaan
  while (WiFi.status() != WL_CONNECTED) {  //Loop which makes a point every 500ms until the connection process has finished
    delay(1000);
    Serial.print(".");
    // Jika waktu percobaan lebih dari 10 detik, kirimkan pesan ke ATmega
    if (millis() - startTime > 10000) {
      Serial.println("No WiFi");
      return;
    }
  }
  // Serial.println("\nConnected to WiFi");
  Serial.println("Connected");  // Kirim pesan "Connected" saat pertama kali terhubung
}

void reconnectWiFi() {
  WiFi.disconnect();
  WiFi.begin(ssid, pass);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    if (millis() - startTime > 10000) {
      Serial.println("\nNo Wifi");
      return;
    }
  }
  Serial.println("\nConnected");
}
// void reconnectWiFi() {
//   // Serial.println("No WiFi");
//   WiFi.disconnect();
//   WiFi.begin(ssid, pass);
//   delay(1000);
// }


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
  if (text == "" || text.toFloat() == 0) return false;  // Periksa apakah kosong atau 0
  if (text.indexOf("x") > 0 || text.indexOf("!") > 0 || text.indexOf("#") > 0 || text.indexOf("$") > 0 || text.indexOf("%") > 0 || text.indexOf("&") > 0) {
    return false;  // Periksa karakter tidak valid
  }
  return true;
}
