// RTC
#include <Wire.h>
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);
char datetime[20];
int thn, bln, tgl, jam, mnt, dtk;  // variable untuk kirim data

// DS
#include <OneWire.h>
#include <DallasTemperature.h>      // Library DS18B20
#define DS 6                        // pin data 
OneWire oneWire(DS);
DallasTemperature sensors(&oneWire);
float temperatureds;                // variabel pembacaan suhu air

// PH
#include "DFRobot_PH.h"             // library sensor pH
#include "EEPROM.h"
#define PH_PIN A0                   // pin data terhubung ke pin A0
float voltageph, phValue;           // variable voltage dan nilai pH
DFRobot_PH ph;
float totalph, averageph;
#define KVALUEADDR 0xFF             // alamat memory
String kirim, kondisi;

int h;
#define relaysampling 8

// Timer
unsigned long lastTime = 0;
unsigned long minutes = 0;

// State Machine
enum State {
  IDLE,
  SAMPLING,
  KIRIM_DATA
};
State currentState = IDLE;

unsigned long timeNow = 0;
bool pesan_terkirim = false;

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
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }
  
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
}

// Setup
void setup() {
  Serial.begin(115200);
  pinMode(relaysampling, OUTPUT);
  digitalWrite(relaysampling, HIGH);
  Wire.begin();
  sensors.begin();
  ph.begin();
  
  for (byte i = 0; i < 8; i++) {  
    EEPROM.write(KVALUEADDR + i, 0xFF);
  }
  
  // RTC setup
  Rtc.Begin();
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

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  
  waktuku();
  currentState = IDLE;  // Set state ke IDLE pada awal program
}

// Loop
void loop() {
  switch (currentState) {
    case IDLE:
      waktuku();
      DSTEMP();

      
      // Cek apakah waktu sampling telah tercapai (2 menit)
      if (millis() - lastTime > 60000) { 
        minutes++;
        lastTime = millis();
      }

      if (minutes == 1) {  // Jika 10 menit telah tercapai, masuk ke state SAMPLING
        currentState = SAMPLING;
      }
      break;

    case SAMPLING:
      digitalWrite(relaysampling, LOW);  // Aktifkan relay
      delay(1000);
      digitalWrite(relaysampling, HIGH); // Matikan relay setelah 3 detik

      // Lakukan pengambilan data sensor
      static unsigned long timepoint = millis();
      if (millis() - timepoint > 1000U) { // Interval sampling 1 detik
        timepoint = millis();
        
        voltageph = analogRead(PH_PIN) / 1024.0 * 5000; // Baca tegangan sensor pH
        phValue = ph.readPH(voltageph, temperatureds);  // Konversi tegangan ke nilai pH
        waktuku();
        DSTEMP();
      }

      // Set state ke KIRIM_DATA setelah sampling selesai
      currentState = KIRIM_DATA;
      break;

    case KIRIM_DATA:
      // Kirim data hasil pengukuran
      kirim_data();
      
      // Reset timer dan kembali ke IDLE setelah mengirim data
      minutes = 0;
      pesan_terkirim = true;
      currentState = IDLE;
      break;
  }
  // Kalibrasi sensor pH (melalui perintah serial)
    ph.calibration(voltageph, temperatureds);
}

// Sensor
void DSTEMP() {
  sensors.requestTemperatures();               // Perintah untuk mendapatkan suhu
  temperatureds = sensors.getTempCByIndex(0);  // Baca suhu dari DS18B20
}

void kirim_data() {
  kirim = "";
  kirim += phValue;        kirim += "@";
  kirim += temperatureds;  kirim += "#";
  kirim += datetime;       kirim += "$";
  Serial.print(kirim);
}
