//Board DOIT ESP32 DEVKIT V1

#include <EEPROM.h>
#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Button.h>           //Memanggil library Push Button
#include <ESP32Servo.h>       //library fix servo
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define pwindow 4             //pin Motor power window
#define MOTOR 26              //pin motor sama dengan brushless
#define EEPROM_SIZE 512       // banyak alamat eeprom yang disediakan

//#define escpin 26 // motor brushless
//Servo ESC; //motor driver brushless

LiquidCrystal_I2C lcd(0x27, 16, 2);

RTC_DS3231 rtc;
String hari[8] = {"Minggu ", "Senin ", "Selasa ", "Rabu ", "Kamis ", "Jumat ", "Sabtu ", "Minggu "};

//Servo
static const int servoPin = 5;
Servo servo1;
//int x = 3000;//delay motor
int angle = 0;
int angleStep = 5;
int potValue;

// setting PWM properties untuk mapping motor driver BTS
const int freq = 50;
const int ledChannel = 2;//channel saat mapping, bukan pin
const int kuatChannel = 3; //channel saat mapping, bukan pin
//const int buzzChannel = 3;
const int resolution = 10; //Resolution 8, 10, 12, 15

unsigned long previousMillis = 0;

int angleMin = 0;
int angleMax = 180;
int okButton = 13; //12
int upButton = 14; //18
int downButton = 27; //19
int cancelButton = 12; //15
int menu, state, stahun, sbulan, stanggal, sjam, smenit, sdetik, delayButton = 200, fresh;

//unsigned long rpt = REPEAT_FIRST;
int durasi, lv;     //durasi waktu keluaran, jauh lemparan
int varjam[13];     //setting jam pada alarm
int varmenit[13];   //setting menit pada alarm
int var[8];         // untuk on/off harian
int sv[13];         // on/off alarm

//EEPROM, penyimpanan variabel
int eDurasi = {0};
int eLv = {2};
int eJam[13] = {4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28}; //memo alarm jam
int eMenit[13] = {28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52}; // memo alarm menit
int eVar[8] = {54, 56, 58, 60, 62, 64, 66, 68};
int eSv[13] = {70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94};//memo save slot alarm

int i = 1,
    j, //power
    m,
    m2 = 0,//varjam, varmenit, //set alarm jam menit
    h,  //hari
    tl, b, tn, tr, pt, pi, per, dey, kuatlempar, w, x,y,z;

#define ok     !digitalRead(okButton)
#define cancel !digitalRead(cancelButton)
#define up     !digitalRead(upButton)
#define down   !digitalRead(downButton)

enum sub {
  awal,  setWaktu,  settgl,
  menu1,  menu2,  menu3,  menu4,  menu5,  menu6,  menu7,  menu8,
  sub1a,  sub1b,
  alarm1,  alarm2,  alarm3,  alarm4,  alarm5,  alarm6,  alarm7,  alarm8,  alarm9,  alarm10,  alarm11,  alarm12,
  takaran1,
  ulang1,  ulang2,
  costum0,
  reset1,
  quest,
  tanggal,  bulan,  tahun,  jam,  menit,  detik,
  salarmjam,  salarmmenit,  savewaktu,  savealarm,  savetakaran,  saveperulangan,  savecostum,
  costumhari0,  costumhari1,  costumhari2,  costumhari3,  costumhari4,  costumhari5,  costumhari6,
  takar,  level,  level1
};

//String serverName = "https://omahiot.net/autofeeder/autofeeder.php?m=read_config&sn=2021110010";
String serverName = "https://omahiot.net/autofeeder/autofeeder.php?m=read_config&sn=382582";
unsigned long lastTime = 0;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

//variabel untuk ambil data dari server
float tanggalw, bulanw, tahunw, jamw, menitw, takaranw, var0, var1, var2, var3, var4, var5, var6, var7,
      varjam1, varjam2, varjam3, varjam4, varjam5, varjam6, varjam7, varjam8, varjam9, varjam10, varjam11, varjam12,
      varmenit1, varmenit2, varmenit3, varmenit4, varmenit5, varmenit6, varmenit7, varmenit8, varmenit9, varmenit10, varmenit11, varmenit12,
      power1, power2, power3, power4, power5, power6, power7, power8, power9, power10, power11, power12, lv1, lv2, lv3, lv4;

//koneksi
bool isWifiConnected, isOnline;
WiFiManager wm;

void setup() {
  Serial.begin(115200);
  Wire.setClock(10000);
  servo1.attach(servoPin);

  pinMode(pwindow, OUTPUT);
  pinMode(MOTOR , OUTPUT);
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(cancelButton, INPUT_PULLUP);
  pinMode(okButton, INPUT_PULLUP);
  pinMode(2, OUTPUT);
  Wire.begin(21, 22);

  EEPROM.begin(EEPROM_SIZE);

  //rtc.adjust(DateTime(2020, 7, 12, 15, 20, 50));
  lcd.init();
  lcd.backlight();

  lcd.setCursor(2, 0);  lcd.print("AutoFeeder");
  lcd.setCursor(4, 1);  lcd.print("OmahIoT");

  //koneksi
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  wm.setConfigPortalTimeout(1);
  isWifiConnected = wm.autoConnect("Autofeed", "12345678"); //AP
  if (!isWifiConnected) {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
  //  state = awal;

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  ledcSetup(kuatChannel, freq, resolution);

  //  ledcAttachPin(LED, ledChannel);
  ledcAttachPin(MOTOR, ledChannel);
  ledcAttachPin(pwindow, kuatChannel);

  //Brushless
  //  ESC.attach(escChannel, 1000, 2000); // (pin, min pulse width, max pulse width in microseconds)
  //  ESC.attach(escpin, 1000, 2000); //pin26
  //  potValue = map(0, 0, 1023, 1000, 2000);   // scale it to use it with the servo library (value between 0 and 180)
  //  ESC.writeMicroseconds(0);

  //baca dan ambil data dari eeprom
  durasi = EEPROM.read(eDurasi);
  lv = EEPROM.read(eLv);
  eVar[0] = eVar[7];

  for (i = 1; i < 13; i++) {
    varjam[i] = EEPROM.read(eJam[i]);
    varmenit[i] = EEPROM.read(eMenit[i]);
    sv[i] = EEPROM.read(eSv[i]);
  }

  for (x = 0; x < 8; x++) {
    var[x] = EEPROM.read(eVar[x]);
  }

    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    //Uncomment the below line to set the initial date and time
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  dey = (durasi * 1000); //delay untuk durasi keluaran pakan
  isOnline = isWifiConnected;
  lcd.clear();
  state = awal;
}

void loop() {
  DateTime now = rtc.now();
  
  if (isOnline == true) {online();}
  
  if (WiFi.status() != WL_CONNECTED) {
    isWifiConnected = false;
    isOnline == false;
  } 
  else {    isWifiConnected = true;  }
  
  switch (state) {
    case awal:
      //Tampilan Awal
      lcd.setCursor(0, 0);      lcd.print(hari[now.dayOfTheWeek()]);      lcd.print(" ");
      lcd.setCursor(7, 0);      sjam = now.hour();      if (sjam < 10) {lcd.print("0");     lcd.print(sjam);}
      else {lcd.print(sjam);}
      
      lcd.print(':');

      smenit = now.minute();
      lcd.setCursor(10,0);
      if (smenit < 10) {lcd.print("0");        lcd.print(smenit);}
      else {lcd.print(smenit);}

      lcd.print(':');
      
      sdetik = now.second();
      lcd.setCursor(13, 0);
      if (sdetik < 10) {lcd.print("0");        lcd.print(sdetik);}
      else {lcd.print(sdetik);}
      lcd.print("     ");
      
      lcd.setCursor(3, 1);
      stanggal = now.day();
      if (stanggal < 10) {lcd.print("0");      lcd.print((stanggal), DEC);}
      else {lcd.print((stanggal), DEC);}

      lcd.print('/');

      sbulan = now.month();
      if ((sbulan) < 10) {lcd.print("0");      lcd.print((sbulan), DEC);}
      else {lcd.print((sbulan), DEC);}         
      
      lcd.print('/');

      stahun = now.year();
      lcd.print((stahun), DEC);
      lcd.print("   ");

      //delay(delayButton);
      if (ok)     {delay(delayButton);   state = menu1;}        // masuk ke bagian menu setting waktu dan alarm
      if (cancel) {delay(delayButton);}
      if (up)     {delay(delayButton);}
      if (down)   {delay(delayButton);}
    break;

    //menu1 setting waktu
    case menu1:
      lcd.setCursor(0, 0);      lcd.print(">Set Waktu      ");
      lcd.setCursor(0, 1);      lcd.print(" Set Alarm      ");
      if (ok)     {delay(delayButton);      state = tanggal;}     // masuk pengaturan tanggal
      if (cancel) {delay(delayButton);      state = awal;}       // kembali ke halaman awal
      if (up)     {delay(delayButton);      state = menu8;}      // ke bagian menu wifi dan mode
      if (down)   {delay(delayButton);      state = menu2;}      // ke >Set Alarm
    break;

    //setting tanggal
    case tanggal:
      lcd.setCursor(0, 0);      lcd.print(" v              ");
      lcd.setCursor(0, 1);
      if ((tl) < 10) {lcd.print("0");        lcd.print(tl);}      // tanggal
      else {lcd.print(tl);}
      
      lcd.setCursor(2, 1);      lcd.print("/");
      if ((b) < 10) {lcd.print("0");        lcd.print(b);}
      else {lcd.print(b);}                                        // bulan
      
      lcd.setCursor(5, 1);      lcd.print("/");      lcd.print(stahun + tn);                                     // tahun
      
      lcd.setCursor(11, 1);
      if (j < 10) {lcd.print("0");        lcd.print(j);}          // jam
      else {lcd.print(j);}

      lcd.setCursor(13, 1);      lcd.print(":");
      if (m < 10) {lcd.print("0");        lcd.print(m);}          // menit
      else {lcd.print(m);}
      lcd.print("      ");

      if (ok)     {delay(delayButton);     state = bulan;}        // masuk ke pengaturan bulan
      if (cancel) {delay(delayButton);     rtc.adjust(DateTime((stahun + tn), (b), (tl), (j), (m), 0));        state = menu1;}
      if (up)     {delay(delayButton);     tl++; if (tl > 31)tl = 1;}
      if (down)   {delay(delayButton);     tl--; if (tl < 1)tl = 31;}
    break;

    //setting bulan
    case bulan:
      lcd.setCursor(0, 0);      lcd.print("    v           ");
      lcd.setCursor(0, 1);      if ((tl) < 10) {lcd.print("0");        lcd.print(tl);}        // tanggal
      else {lcd.print(tl);}

      lcd.setCursor(2, 1);      lcd.print("/");
      if ((b) < 10) {lcd.print("0");        lcd.print(b);}
      else {lcd.print(b);}

      lcd.setCursor(5, 1);      lcd.print("/");
      lcd.print(stahun + tn);

      lcd.setCursor(11, 1);
      if (j < 10) {lcd.print("0");        lcd.print(j);}
      else {lcd.print(j);}

      lcd.setCursor(13, 1);      lcd.print(":");
      if (m < 10) {lcd.print("0");        lcd.print(m);}
      else {lcd.print(m);}
      lcd.print("      ");

      if (ok)     {delay(delayButton);     state = tahun;}
      if (cancel) {delay(delayButton);     state = tanggal;}
      if (up)     {delay(delayButton);     b++; if (b > 12)b = 1;}
      if (down)   {delay(delayButton);     b--; if (b < 1)b = 12;}
      //EEPROM.commit();
    break;

    //setting tahun
    case tahun:
      lcd.setCursor(0, 0);      lcd.print("         v      ");
      lcd.setCursor(0, 1);      if ((tl) < 10) {lcd.print("0");        lcd.print(tl);}
      else {lcd.print(tl);}

      lcd.setCursor(2, 1);      lcd.print("/");

      if ((b) < 10) {        lcd.print("0");        lcd.print(b);}
      else {lcd.print(b);}

      lcd.setCursor(5, 1);      lcd.print("/");      lcd.print(stahun + tn);

      lcd.setCursor(11, 1);      if (j < 10) {lcd.print("0");        lcd.print(j);}
      else {lcd.print(j);}

      lcd.setCursor(13, 1);      lcd.print(":");      if (m < 10) {lcd.print("0");        lcd.print(m);}
      else {lcd.print(m);}
      lcd.print("      ");

      if (ok)     {delay(delayButton);        state = jam;}
      if (cancel) {delay(delayButton);        state = bulan;}
      if (up)     {delay(delayButton);        tn++;}
      if (down)   {delay(delayButton);        tn--;}
    break;

    //setting jam
    case jam:
      lcd.setCursor(0, 0);      lcd.print("            v   ");
      lcd.setCursor(0, 1);      if ((tl) < 10) {lcd.print("0");        lcd.print(tl);}
      else {lcd.print(tl);}

      lcd.setCursor(2, 1);      lcd.print("/");      if ((b) < 10) {lcd.print("0");        lcd.print(b);}
      else {lcd.print(b);}

      lcd.setCursor(5, 1);      lcd.print("/");      lcd.print(stahun + tn);

      lcd.setCursor(11, 1);      if (j < 10) {lcd.print("0");        lcd.print(j);}
      else {lcd.print(j);}

      lcd.setCursor(13, 1);      lcd.print(":");      if (m < 10) {lcd.print("0");        lcd.print(m);}
      else {lcd.print(m);}
      lcd.print("      ");

      if (ok)     {delay(delayButton);        state = menit;}
      if (cancel) {delay(delayButton);        state = tahun;}
      if (up)     {delay(delayButton);        j++; if (j > 23)j = 0;}
      if (down)   {delay(delayButton);        j--; if (j < 0)j = 23;}
    break;

    //setting menit
    case menit:
      lcd.setCursor(0, 0);      lcd.print("               v");
      lcd.setCursor(0, 1);      if ((tl) < 10) {lcd.print("0");        lcd.print(tl);}
      else {lcd.print(tl);}

      lcd.setCursor(2, 1);      lcd.print("/");      if ((b) < 10) {lcd.print("0");        lcd.print(b);}
      else {lcd.print(b);}

      lcd.setCursor(5, 1);      lcd.print("/");      lcd.print(stahun + tn);

      lcd.setCursor(11, 1);      if (j < 10) {lcd.print("0");        lcd.print(j);}
      else {lcd.print(j);}

      lcd.setCursor(13, 1);      lcd.print(":");      if (m < 10) {lcd.print("0");        lcd.print(m);      }
      else {lcd.print(m);}
      lcd.print("      ");

      if (ok)     {delay(delayButton);        rtc.adjust(DateTime((stahun + tn), (b), (tl), (j), (m), 0));        state = awal;}
      if (cancel) {delay(delayButton);        state = jam;}
      if (up)     {delay(delayButton);        m++; if (m > 59)m = 0;};
      if (down)   {delay(delayButton);        m--; if (m < 0)m = 59;};
    break;

    //menu2 Setting alarm
    case menu2:
      lcd.setCursor(0, 0);      lcd.print(" Set Waktu      ");
      lcd.setCursor(0, 1);      lcd.print(">Set Alarm      ");

      if (ok)    {
        delay(delayButton);
        //var[m] = EEPROM.read(eVar[48]);
        m2 = 1;
        state = alarm1;                                           // setting alarm ke-
      };
      if (cancel) {delay(delayButton);        state = awal;};     // kembali ke menu awal
      if (up)     {delay(delayButton);        state = menu1;};    // ke menu setting waktu
      if (down)   {delay(delayButton);        state = menu3;};    // ke menu eDurasi
    break;

    //Setting alarm ke-
    case alarm1:
      lcd.setCursor(0, 0);      lcd.print("      v         ");
      lcd.setCursor(0, 1);      lcd.print("Alarm ");              lcd.print(m2);      lcd.print("  ");
      lcd.setCursor(9, 1);      if (varjam[m2] < 10) {lcd.print("0");        lcd.print(varjam[m2]);}
      else {lcd.print(varjam[m2]);}

      lcd.setCursor(11, 1);      lcd.print(":");
      if (varmenit[m2] < 10) {lcd.print("0");        lcd.print(varmenit[m2]);       lcd.print("   ");}
      else {lcd.print(varmenit[m2]);        lcd.print("   ");}

      if (ok)     {delay(delayButton);        state = salarmjam;};
      if (cancel) {delay(delayButton);        state = menu2;};
      if (up)     {delay(delayButton);        m2++; if (m2 > 12)m2 = 1;};
      if (down)   {delay(delayButton);        m2--; if (m2 < 1 )m2 = 12;};
      // EEPROM.commit();
    break;

    //setting jam pada alarm ke-
    case salarmjam:
      lcd.setCursor(0, 0);      lcd.print("          v     ");
      lcd.setCursor(0, 1);      lcd.print("Alarm ");      lcd.print(m2);      lcd.print("  ");

      lcd.setCursor(9, 1);
      if (varjam[m2] < 10) {lcd.print("0");        lcd.print(varjam[m2]);}
      else {lcd.print(varjam[m2]);}

      lcd.setCursor(11, 1);      lcd.print(":");
      if (varmenit[m2] < 10) {lcd.print("0");        lcd.print(varmenit[m2]);}
      else {lcd.print(varmenit[m2]);}

      if (ok)     {delay(delayButton);        state = salarmmenit;};
      if (cancel) {delay(delayButton);        i = 0;        state = savealarm;};      // menyimpan jam dan menit alarm
      if (up)     {delay(delayButton);        varjam[m2]++; if (varjam[m2] > 23)varjam[m2] = 0;};
      if (down)   {delay(delayButton);        varjam[m2]--; if (varjam[m2] < 0)varjam[m2] = 23;};
    break;

    //setting menit pada alarm ke-
    case salarmmenit:
      lcd.setCursor(0, 0);      lcd.print("             v  ");
      lcd.setCursor(0, 1);      lcd.print("Alarm ");      lcd.print(m2);      lcd.print("  ");
      lcd.setCursor(9, 1);      if (varjam[m2] < 10) {lcd.print("0");         lcd.print(varjam[m2]);}
      else {lcd.print(varjam[m2]);}

      lcd.setCursor(11, 1);      lcd.print(":");      if (varmenit[m2] < 10) {lcd.print("0");        lcd.print(varmenit[m2]);        lcd.print(" ");}
      else {lcd.print(varmenit[m2]);        lcd.print(" ");}

      if (ok)     {delay(delayButton);        state = salarmjam;}
      if (cancel) {delay(delayButton);        i = 0;        state = savealarm;}
      if (up)     {delay(delayButton);        varmenit[m2]++; if (varmenit[m2] > 59)varmenit[m2] = 0;}
      if (down)   {delay(delayButton);        varmenit[m2]--; if (varmenit[m2] < 0)varmenit[m2] = 59;}
      EEPROM.commit();
    break;

    //save pengaturan jam & menit alarm ke EEPROM
    case savealarm:
      lcd.setCursor(0, 0);      lcd.print("Apa Anda Yakin ?");
      lcd.setCursor(0, 1);      if (i < 1) {lcd.print(">Tidak      Iya ");}
      else {lcd.print(" Tidak     >Iya ");}

      if (ok)    {
        delay(delayButton);
        if (i > 0) {
          sv[m2] = 1;
          EEPROM.write(eSv[m2], sv[m2]);
          EEPROM.write(eJam[m2], varjam[m2]);
          EEPROM.write(eMenit[m2], varmenit[m2]);
          EEPROM.commit();
          Serial.print("Data saved to EEPROM");
          state = awal;
        }

        else {
          // sv[m2] = 0;
          // varjam[m2] = 24;
          // varmenit[m2] = 60;
          // EEPROM.write(eJam[m2], varjam[m2]);
          // EEPROM.write(eMenit[m2], varmenit[m2]);
          // EEPROM.write(eSv[m2], sv[m2]);
          // EEPROM.commit();
          state = alarm1;
        }

      };
      if (cancel) {      };
      if (up)     {delay(delayButton);       i--; if (i < 0)i = 1;};
      if (down)   {delay(delayButton);       i++; if (i > 1)i = 0;};
    break;

    //menu3 eDurasi
    case menu3:
      lcd.setCursor(0, 0);      lcd.print(">Takaran        ");
      lcd.setCursor(0, 1);      lcd.print(" Perulangan     ");
      if (ok)     {delay(delayButton);        state = level;};      // ke setting jauhnya lemparan pakan      
      if (cancel) {delay(delayButton);        state = awal;};       // ke menu awal
      if (up)     {delay(delayButton);        state = menu2;};      // ke set alarm
      if (down)   {delay(delayButton);        state = menu4;};      // ke set alarm aktif di hari apa saja
    break;

    //setting jauhnya lemparan pakan
    case level:
      lcd.setCursor(0, 0);      lcd.print(">Level   ");      lcd.print("0");      lcd.print(lv);      lcd.print("       ");
      lcd.setCursor(0, 1);      lcd.print(" Durasi");        lcd.print("         ");

      if (ok)     {delay(delayButton);        state = level1;};     // ke pengaturan level
      if (cancel) {delay(delayButton);        state = menu3;};      // ke menu edurasi
      if (up)     {delay(delayButton);        state = takar;};      // ke setting lamanya pakan keluar
      if (down)   {delay(delayButton);        state = takar;};      // ke setting lamanya pakan keluar
    break;

    //memilih level lemparan 1 - 5
    case level1:
      lcd.setCursor(0, 0);      lcd.print("     Level      ");
      lcd.setCursor(0, 1);      lcd.print("       ");      lcd.print(lv);      lcd.print("         ");
      if (ok)    {
        delay(delayButton);
        EEPROM.write(eLv, lv);
        EEPROM.commit();
        state = level;

      };
      if (cancel) {      };
      if (up)     {delay(delayButton);        lv++;        if (lv > 4)lv = 1;};
      if (down)   {delay(delayButton);        lv--;        if (lv < 1 )lv = 4;};
    break;

    //setting lamanya pakan keluar
    case takar:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(" Level          ");
      lcd.setCursor(0, 1);
      lcd.print(">Durasi  ");
      if (durasi < 10) {
        lcd.print("0");
        lcd.print(durasi);
      }
      else {
        lcd.print(durasi);
      }
      lcd.print("     ");
      if (ok)    {
        delay(delayButton);
        state = takaran1;
      };
      if (cancel) {
        delay(delayButton);
        state = menu3;
      };
      if (up)    {
        delay(delayButton);
        state = level;
      };
      if (down)  {
        delay(delayButton);
        state = level;
      };
      break;

    //memilih waktu lama keluar (1-20)
    case takaran1:
      lcd.setCursor(0, 0);
      lcd.print(">Durasi ");
      if (durasi < 10) {
        lcd.print("0");
        lcd.print(durasi);
      }
      else {
        lcd.print(durasi);
      }
      //         0123456789012345
      lcd.print(" detik");
      lcd.setCursor(0, 1);
      if (durasi <= 30) {
        //         0123456789012345
        lcd.print("        ");
        if (durasi + 1 < 10) {
          lcd.print("0");
          lcd.print(durasi + 1);
        }
        else {
          lcd.print(durasi + 1);
        }
        //lcd.print(durasi+1);
        lcd.print("      ");
      }
      else {
        //         0123456789012345
        lcd.print("        01");
      }

      if (ok)    {
        delay(delayButton);
        EEPROM.write(eDurasi, durasi);
        EEPROM.commit();
        state = level;

      };
      if (cancel) {
        //        delay(delayButton);
        //        //save jam & menit
        //        state = menu3;
      };
      if (up)    {
        delay(delayButton);
        durasi++;
        if (durasi > 30)durasi = 1;
        dey = (durasi * 1000);
      };
      if (down)  {
        delay(delayButton);
        durasi--;
        if (durasi < 1 )durasi = 30;
        dey = (durasi * 1000);
      };
      break;

    //menu4 menghidupkan alarm di hari apa saja
    case menu4:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(" Takaran        ");
      lcd.setCursor(0, 1);
      lcd.print(">Perulangan     ");
      if (ok)    {
        delay(delayButton);
        //i = 0; j = 0;
        state = ulang1;
      };
      if (cancel) {
        delay(delayButton);
        state = awal;
      };
      if (up)    {
        delay(delayButton);
        state = menu3;
      };
      if (down)  {
        delay(delayButton);
        state = menu5;
      };
      break;

    //memilih mode everyday (tiap hari nyala)
    case ulang1:
      lcd.setCursor(0, 0);
      //         0123456789012345
      lcd.print(">Everyday       ");
      lcd.setCursor(0, 1);
      lcd.print(" Costum         ");

      if (ok)    {
        delay(delayButton);

        for (per = 1; per < 8; per++) {
          var[per] = 1;
          var[0] = var[7];
          eVar[0] = eVar[7];
          EEPROM.write(eVar[per], var[per]);
          EEPROM.write(eVar[7], var[7]);
          EEPROM.commit();
        }
        state = menu4;
      };

      if (cancel) {
        delay(delayButton);
        state = menu4;
      };
      if (up)    {
        delay(delayButton);
        state = ulang2;
      };
      if (down)  {
        delay(delayButton);
        state = ulang2;
      };
      break;

    //memilih mode costum
    case ulang2:
      lcd.setCursor(0, 0);
      //         0123456789012345
      lcd.print(" Everyday       ");
      lcd.setCursor(0, 1);
      lcd.print(">Costum         ");

      if (ok)    {
        delay(delayButton);
        state = costum0;
      };
      if (cancel) {
        delay(delayButton);
        state = menu4;
      };
      if (up)    {
        delay(delayButton);
        state = ulang1;
      };
      if (down)  {
        delay(delayButton);
        state = ulang1;
      };
      break;

    //setting pada hari apa saja alarm akan di hidupkan
    case costum0:
      lcd.setCursor(0, 0);
      //         0123456789012345
      lcd.print(">");
      lcd.print(hari[per]);
      lcd.print("    ");

      lcd.setCursor(12, 0);
      if (var[per] < 1) {
        lcd.print("OFF");
      }
      else {
        lcd.print("ON ");
      }
      lcd.setCursor(0, 1);
      //         0123456789012345
      lcd.print(" ");
      lcd.print(hari[per + 1]);
      lcd.print("    ");

      lcd.setCursor(12, 1);

      if (var[per + 1] < 1) {
        lcd.print("OFF");
      }
      else {
        lcd.print("ON ");
      }

      if (ok)    {
        delay(delayButton);
        var[per]++;
        if (var[per] > 1 )var[per] = 0;
        var[0] = var[7];
        eVar[0] = eVar[7];
        EEPROM.write(eVar[per], var[per]);
        EEPROM.commit();
        //state = costumhari0;

      };
      if (cancel) {
        delay(delayButton);
        //save jam & menit
        state = ulang2;
      };
      if (up)    {
        delay(delayButton);
        per--;
        if (per < 0 )per = 6;

      };
      if (down)  {
        delay(delayButton);
        per++;
        if (per > 6 )per = 0;

      };
      break;

    //mengatur semua variabel ke pengaturan pabrik, kecuali tanggal hari bulan tahun (RTC)
    case menu5:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(">Reset          ");
      lcd.setCursor(0, 1);
      lcd.print(" Tes            ");
      if (ok)    {
        delay(delayButton);
        i = 0;
        state = quest;
      };
      if (cancel) {
        delay(delayButton);
        state = awal;
      };
      if (up)    {
        delay(delayButton);
        state = menu4;
      };
      if (down)  {
        delay(delayButton);
        state = menu6;
      };
      break;

    //persetujuan
    case quest:
      lcd.setCursor(0, 0);
      //         0123456789012345
      lcd.print("Apa Anda Yakin ?");
      lcd.setCursor(0, 1);
      if (i <= 0) {
        //         0123456789012345
        lcd.print(">Tidak      Iya ");
      }
      if (i > 0) {
        //         0123456789012345
        lcd.print(" Tidak     >Iya ");
      }

      if (ok)    {
        delay(delayButton);
        durasi = 5;
        EEPROM.write(eDurasi, durasi);
        EEPROM.commit();
        lv = 1;
        EEPROM.write(eLv, lv);
        EEPROM.commit();

        if (i > 0) {

          for (fresh = 1; fresh < 13; fresh++) {
            sv[fresh] = 0;
            varjam[fresh] = 0;  varmenit[fresh] = 0;
            EEPROM.write(eJam[fresh], varjam[fresh]);
            EEPROM.write(eMenit[fresh], varmenit[fresh]);
            EEPROM.write(eSv[fresh], sv[fresh]);
            EEPROM.commit();
          }

          for (w = 0; w < 8; w++) {
            var[w] = 1;
            EEPROM.write(eVar[w], var[w]);
            EEPROM.commit();
          }

          state = awal;
        }
        if (i < 1) {
          state = menu5;
        }
      };

      if (cancel) {
        delay(delayButton);
        //save jam & menit
        //state = menu5;
      };
      if (up)    {
        delay(delayButton);
        i--; if (i < 0)i = 1;
      };
      if (down)  {
        delay(delayButton);
        i++; if (i > 1)i = 0;
      };
    break;

    //alat langsung mengeluarkan pakan berdasarkan pengaturan durasi dan level jauh nya pakan dilontarkan
    case menu6:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(" Reset          ");
      lcd.setCursor(0, 1);
      lcd.print(">Tes            ");
      if (ok)    {
        delay(delayButton);
        motordc();
        state = awal;
      };
      if (cancel) {
        delay(delayButton);
        state = awal;
      };
      if (up)    {
        delay(delayButton);
        state = menu5;
      };
      if (down)  {
        delay(delayButton);
        state = menu7;
      };
    break;

    case menu7:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(">WiFi: ");
      lcd.setCursor(7, 0);
      if (!isWifiConnected) {
        lcd.print("Terputus ");
        if (ok)    {
          delay(delayButton);
          connectingWifi();
          if (WiFi.status() != WL_CONNECTED) {isWifiConnected = false;} 
          else {isWifiConnected = true;}
          state = menu7;
        };
      }
      else {
        //if you get here you have connected to the WiFi
        lcd.print("Terhubung");
      }
      lcd.setCursor(0, 1);
      lcd.print(" Mode: ");
      lcd.setCursor(7, 1);
      if (!isOnline) {lcd.print("Offline ");}
      else {
        //if you get here you have connected to the WiFi
        lcd.print("Online  ");
      }
      if (cancel) {
        delay(delayButton);
        state = awal;
      };
      if (up)    {
        delay(delayButton);
        state = menu6;
      };
      if (down)  {
        delay(delayButton);
        state = menu8;
      };
      break;

    case menu8:
      //         0123456789012345
      lcd.setCursor(0, 0);
      lcd.print(" WiFi: ");
      lcd.setCursor(7, 0);
      if (!isWifiConnected) {
        lcd.print("Terputus ");
      }
      else {
        //if you get here you have connected to the WiFi
        lcd.print("Terhubung");
        if (ok)    {
          delay(delayButton);
          isOnline = !isOnline;
          state = menu8;
        };
      }
      lcd.setCursor(0, 1);
      lcd.print(">Mode: ");
      lcd.setCursor(7, 1);

      if (!isOnline) {lcd.print("Offline ");}
      else {
        //if you get here you have connected to the WiFi
        lcd.print("Online  ");
      }
      
      if (cancel) {
        delay(delayButton);
        state = awal;
      };
      if (up)    {
        delay(delayButton);
        state = menu7;
      };
      if (down)  {
        delay(delayButton);
        state = menu1;
      };
    break;
  }

  //PROGRAM EKSEKUSI
  // mengatur level jauhnya lemparan pakan untuk coding mapping
  if (lv == 1) {kuatlempar = 290;}
  if (lv == 2) {kuatlempar = 400;}
  if (lv == 3) {kuatlempar = 600;}
  if (lv == 4) {kuatlempar = 800;}

  var[8] = var[0];
  for (y = 0; y < 8; y++) {
    for (z = 0; z < 12; z++) {
      //      perulangan          hari          on/off      jam               menit         detik
      if ((var[y] > 0) && (hari[y] == hari[now.dayOfTheWeek()]) && (sv[z] == 1) && (sjam == varjam[z]) && (smenit == varmenit[z]) && (sdetik == 0)) {
        motordc();
      }
    }
  }
}

void motordc() {
  for (int angle = 360; angle >= 135; angle -= angleStep) {//pintu menutup
    servo1.write(angle);
    delay(40);
  }

  int kecepatan = map(kuatlempar, 0, 1000, 0, 255);
  analogWrite(MOTOR, kecepatan);//kekuatan motor ulir saat nyala
  analogWrite(pwindow, 0);//kekuatan motor pelempar saat nyala

  delay(dey);

  analogWrite(MOTOR, 0);
  analogWrite(pwindow, 0);
  delay(2000);

  for (int angle = 135; angle <=   360 ; angle += angleStep) { //pintu membuka, angle awal disesuaikan dengan posisi awal pintu tertutup (dicoba coba secara manual/kira kira dulu)
    servo1.write(angle);
    delay(40);
  }
}
void online() {
  EEPROM.commit();
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status

    HTTPClient http;

    String serverPath = serverName;

    // Your Domain name with URL path or IP address with path
    http.begin(serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString(); //data thd dr server
      Serial.println(payload);
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      
      tanggalw = doc["tanggal"];
      bulanw = doc["bulan"];
      tahunw = doc["tahun"];
      jamw = doc["jam"];
      menitw = doc["menit"];
      takaranw = doc["takaran"];
      var1 = doc["var1"];
      var2 = doc["var2"];
      var3 = doc["var3"];
      var4 = doc["var4"];
      var5 = doc["var5"];
      var6 = doc["var6"];
      var7 = doc["var7"];
      
      varjam1 = doc["varjam1"];
      varjam2 = doc["varjam2"];
      varjam3 = doc["varjam3"];
      varjam4 = doc["varjam4"];
      varjam5 = doc["varjam5"];
      varjam6 = doc["varjam6"];
      varjam7 = doc["varjam7"];
      varjam8 = doc["varjam8"];
      varjam9 = doc["varjam9"];
      varjam10 = doc["varjam10"];
      varjam11 = doc["varjam11"];
      varjam12 = doc["varjam12"];
      
      varmenit1 = doc["varmenit1"];
      varmenit2 = doc["varmenit2"];
      varmenit3 = doc["varmenit3"];
      varmenit4 = doc["varmenit4"];
      varmenit5 = doc["varmenit5"];
      varmenit6 = doc["varmenit6"];
      varmenit7 = doc["varmenit7"];
      varmenit8 = doc["varmenit8"];
      varmenit9 = doc["varmenit9"];
      varmenit10 = doc["varmenit10"];
      varmenit11 = doc["varmenit11"];
      varmenit12 = doc["varmenit12"];
      
      power1 = doc["power1"];
      power2 = doc["power2"];
      power3 = doc["power3"];
      power4 = doc["power4"];
      power5 = doc["power5"];
      power6 = doc["power6"];
      power7 = doc["power7"];
      power8 = doc["power8"];
      power9 = doc["power9"];
      power10 = doc["power10"];
      power11 = doc["power11"];
      power12 = doc["power12"];
      
      lv1 = doc["lvw"];
      
      // Serial.println(tanggalw);      Serial.println(bulanw);      Serial.println(tahunw);      Serial.println(jamw);      Serial.println(menitw);      Serial.println(takaranw);      
      // Serial.println(var1);      Serial.println(var2);      Serial.println(var3);      Serial.println(var4);      Serial.println(var5);      Serial.println(var6);      Serial.println(var7);      
      // Serial.println(varjam1);      Serial.println(varjam2);      Serial.println(varjam3);      Serial.println(varjam4);      Serial.println(varjam5);      Serial.println(varjam6);      Serial.println(varjam7);      Serial.println(varjam8);      Serial.println(varjam9);      Serial.println(varjam10);      Serial.println(varjam11);      Serial.println(varjam12);      
      // Serial.println(varmenit1);      Serial.println(varmenit2);      Serial.println(varmenit3);      Serial.println(varmenit4);      Serial.println(varmenit5);      Serial.println(varmenit6);      Serial.println(varmenit7);      Serial.println(varmenit8);      Serial.println(varmenit9);      Serial.println(varmenit10);      Serial.println(varmenit11);      Serial.println(varmenit12);      
      // Serial.println(power1);      Serial.println(power2);      Serial.println(power3);      Serial.println(power4);      Serial.println(power5);      Serial.println(power6);      Serial.println(power7);      Serial.println(power8);      Serial.println(power9);      Serial.println(power10);      Serial.println(power11);      Serial.println(power12);
      // Serial.println(lv1); //(LV 1 >dibaca el ve satu)

      stanggal = tanggalw;
      sbulan = bulanw;
      stahun = tahunw;
      sjam = jamw;
      smenit = jamw;
      durasi = takaranw;
      dey = (durasi * 1000);

      EEPROM.write(eDurasi, takaranw);
      EEPROM.commit();
      
      var[0] = var0;
      var[1] = var1;
      var[2] = var2;
      var[3] = var3;
      var[4] = var4;
      var[5] = var5;
      var[6] = var6;
      var[7] = var7;

      EEPROM.write(eVar[1], var1);      EEPROM.commit();
      EEPROM.write(eVar[2], var2);      EEPROM.commit();
      EEPROM.write(eVar[3], var3);      EEPROM.commit();
      EEPROM.write(eVar[4], var4);      EEPROM.commit();
      EEPROM.write(eVar[5], var5);      EEPROM.commit();
      EEPROM.write(eVar[6], var6);      EEPROM.commit();
      EEPROM.write(eVar[7], var7);      EEPROM.commit();

      varjam[1] = varjam1;
      varjam[2] = varjam2;
      varjam[3] = varjam3;
      varjam[4] = varjam4;
      varjam[5] = varjam5;
      varjam[6] = varjam6;
      varjam[7] = varjam7;
      varjam[8] = varjam8;
      varjam[9] = varjam9;
      varjam[10] = varjam10;
      varjam[11] = varjam11;
      varjam[12] = varjam12;

      EEPROM.write(eJam[1], varjam1);      EEPROM.commit();
      EEPROM.write(eJam[2], varjam2);      EEPROM.commit();
      EEPROM.write(eJam[3], varjam3);      EEPROM.commit();
      EEPROM.write(eJam[4], varjam4);      EEPROM.commit();
      EEPROM.write(eJam[5], varjam5);      EEPROM.commit();
      EEPROM.write(eJam[6], varjam6);      EEPROM.commit();
      EEPROM.write(eJam[7], varjam7);      EEPROM.commit();
      EEPROM.write(eJam[8], varjam8);      EEPROM.commit();
      EEPROM.write(eJam[9], varjam9);      EEPROM.commit();
      EEPROM.write(eJam[10], varjam10);    EEPROM.commit();
      EEPROM.write(eJam[11], varjam11);    EEPROM.commit();
      EEPROM.write(eJam[12], varjam12);    EEPROM.commit();

      varmenit[1] = varmenit1;
      varmenit[2] = varmenit2;
      varmenit[3] = varmenit3;
      varmenit[4] = varmenit4;
      varmenit[5] = varmenit5;
      varmenit[6] = varmenit6;
      varmenit[7] = varmenit7;
      varmenit[8] = varmenit8;
      varmenit[9] = varmenit9;
      varmenit[10] = varmenit10;
      varmenit[11] = varmenit11;
      varmenit[12] = varmenit12;

      EEPROM.write(eMenit[1], varmenit1);      EEPROM.commit();
      EEPROM.write(eMenit[2], varmenit2);      EEPROM.commit();
      EEPROM.write(eMenit[3], varmenit3);      EEPROM.commit();
      EEPROM.write(eMenit[4], varmenit4);      EEPROM.commit();
      EEPROM.write(eMenit[5], varmenit5);      EEPROM.commit();
      EEPROM.write(eMenit[6], varmenit6);      EEPROM.commit();
      EEPROM.write(eMenit[7], varmenit7);      EEPROM.commit();
      EEPROM.write(eMenit[8], varmenit8);      EEPROM.commit();
      EEPROM.write(eMenit[9], varmenit9);      EEPROM.commit();
      EEPROM.write(eMenit[10], varmenit10);    EEPROM.commit();
      EEPROM.write(eMenit[11], varmenit11);    EEPROM.commit();
      EEPROM.write(eMenit[12], varmenit12);    EEPROM.commit();

      sv[1] = power1;
      sv[2] = power2;
      sv[3] = power3;
      sv[4] = power4;
      sv[5] = power5;
      sv[6] = power6;
      sv[7] = power7;
      sv[8] = power8;
      sv[9] = power9;
      sv[10] = power10;
      sv[11] = power11;
      sv[12] = power12;

      EEPROM.write(eSv[1], power1);      EEPROM.commit();
      EEPROM.write(eSv[2], power2);      EEPROM.commit();
      EEPROM.write(eSv[3], power3);      EEPROM.commit();
      EEPROM.write(eSv[4], power4);      EEPROM.commit();
      EEPROM.write(eSv[5], power5);      EEPROM.commit();
      EEPROM.write(eSv[6], power6);      EEPROM.commit();
      EEPROM.write(eSv[7], power7);      EEPROM.commit();
      EEPROM.write(eSv[8], power8);      EEPROM.commit();
      EEPROM.write(eSv[9], power9);      EEPROM.commit();
      EEPROM.write(eSv[10], power10);    EEPROM.commit();
      EEPROM.write(eSv[11], power11);    EEPROM.commit();
      EEPROM.write(eSv[12], power12);    EEPROM.commit();

      lv = lv1;

      EEPROM.write(eLv, lv1);
      EEPROM.commit();
      http.end();
    }

    else {Serial.println("WiFi Disconnected");}
    
    lastTime = millis();
  }
}

void  connectingWifi() {
  wm.setConfigPortalTimeout(300);
  isWifiConnected = wm.startConfigPortal("Autofeed", "12345678");
  if (!isWifiConnected) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);  
  }
}