// RTC
#include <Wire.h>
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
char datetime[20];
int thn, bln, tgl, jam, mnt, dtk;                               // variable untuk kirim data

// DS
#include <OneWire.h>
#include <DallasTemperature.h>      // Library DS18B20
#define DS 6                        // pin data 
OneWire oneWire(DS);
DallasTemperature sensors(&oneWire);
float temperatureds;                // variabel pembacaan suhu air

//PH
#include "DFRobot_PH.h"             // library sensor pH
#include "EEPROM.h"
#define PH_PIN A0                   // pin data terhubung ke pin A0
float voltageph, phValue;             // variable voltage dan nilai pH
DFRobot_PH ph;
float totalph, averageph;
#define KVALUEADDR 0x0F             // alamat memory
String kirim, kondisi;
bool pesan_terkirim = false;

int h;
#define relaysampling 8

//Timer
unsigned long lastTime = 0;
unsigned long minutes = 0;

// RTC
#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt) {
  
  snprintf(datetime, sizeof(datetime), "%04u-%02u-%02u %02u:%02u:%02u",
           dt.Year(),
           dt.Month(),
           dt.Day(),
           dt.Hour(),
           dt.Minute(),
           dt.Second());
}

void waktuku() {
   if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  //  Serial.println(datetime);

  RtcTemperature temp = Rtc.GetTemperature();
  // temp.Print(Serial);
  // you may also get the temperature as a float and print it
  // Serial.print(temp.AsFloatDegC());
  // Serial.println("C");
  //  delay(1000); // ten seconds
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);                         // Serial.begin
  pinMode(relaysampling, OUTPUT);
  digitalWrite(relaysampling, HIGH);
  Wire.begin();                                 // komunikasi wire
  // Wire.setClock(10000);                         // wire diatur pada clock 10.000
  sensors.begin();                              // DS18B20
  ph.begin();                                   // pH
  for (byte i = 0; i < 8; i++) {  
    EEPROM.write(KVALUEADDR + i, 0xFF);
  }
  
  // RTC
  Rtc.Begin();
  #if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000 /* us /, true / reset_on_timeout */);
  #endif

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);

  if (!Rtc.IsDateTimeValid()) {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning()) {
    
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
    
  }

  RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
    } else if (now > compiled) {
      Serial.println("RTC is newer than compile time. (this is expected)");
    } else if (now == compiled) {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
  

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  delay(250);
  waktuku();
  kirim_data();
}

void loop() {
  waktuku();
  DSTEMP();
  pesan_terkirim = false;

  if (millis() - lastTime > 60000) {
    minutes++;
    lastTime = millis();
  }

  if (minutes == 2) {  //sampling 3 mnt sekali
    digitalWrite(relaysampling, LOW);
    delay(5000);
    digitalWrite(relaysampling, HIGH);
    // Sampling pertama: pH, suhu Air, dan suhu ruangan
    static unsigned long timepoint = millis();
    if(millis()-timepoint>1000U){                  //time interval: 1s
        timepoint = millis();
        //temperature = readTemperature();         // read your temperature sensor to execute temperature compensation
        voltageph = analogRead(PH_PIN)/1024.0*5000;  // read the voltage
        phValue = ph.readPH(voltageph,temperatureds);  // convert voltage to pH with temperature compensation
        waktuku();
        DSTEMP();
    }
    // h = 0;
    // totalph = 0;
    digitalWrite(relaysampling, HIGH);
    minutes = 0;     // Reset nilai menit ke 0
    pesan_terkirim = true;
  }
  else{
    digitalWrite(relaysampling, HIGH);
  }
  if(pesan_terkirim == true){
    kirim_data();    // Kirim data ke esp8266
    pesan_terkirim = false;
  }
}

// Sensor
// DS18B20
void DSTEMP() {
  sensors.requestTemperatures();               // Perintah untuk mendapatkan suhu
  temperatureds = sensors.getTempCByIndex(0);  // gunakan function ByIndex sebagai index pembacaan sensor
}

void kirim_data() {
  kirim = "";
  kirim += phValue;        kirim += "@";
  kirim += temperatureds;    kirim += "#";
  kirim += datetime;         kirim += "$";
  Serial.print(kirim);
}