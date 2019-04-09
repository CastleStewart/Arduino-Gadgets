/*
  Arduino    Component              Arduino     Component
   D0.........                       D7..........RTC CLOCK
   D1.........                       D8..........RELAY #1 (Mic)
   D2.........Rotary Encoder A       D9..........RGB(BLUE)
   D3.........Rotary Encoder B       D10.........RGB(GREEN)
   D4.........DHT11                  D11.........RGB(RED)
   D5.........RTC RST                D12.........RELAY #2 (BT audio)
   D6.........RTC DATA               D13.........Rotary Enc. SWITCH

   A0.........BT data STATUS INPUT   A3.........RESET
   A1.........BT audio pair mode     A4..........LCD Tx
   A2.........                       A5..........LCD Rx

   132 void setup()     615 #
   204 void loop()      677 end
   273 ? help
   462-545 @
   575 ~
*/
#include "DHT.h"
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <RGBLed.h>
#include <HC05.h>
#include <RotaryEncoder.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>

ThreeWire myWire(6,7,5); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

#define countof(a) (sizeof(a) / sizeof(a[0]))


#define ROTARYSTEPS 2
#define ROTARYMIN 0
#define ROTARYMAX 50

// Setup a RoraryEncoder for pins A2 and A3:
RotaryEncoder encoder(2, 3);

// Last known rotary position.
int lastPos = -1;

#define MicRelay1 8
#define BtRelay2 12
#define Reset A3
#define DHTPIN 4
#define DHTTYPE DHT11
#define BTstatus A0
#define REswitch 13
#define Pair A1

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

uint8_t check[8] = {0x0, 0x1 , 0x3, 0x16, 0x1c, 0x8, 0x0};
uint8_t retarrow[8] = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};
uint8_t antenna[9] = {0x15, 0x1F, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x00};
byte castle[9] = {0x1B, 0x0E, 0x04, 0x11, 0x1B, 0x0E, 0x04,0x00};
byte degF[9] = {0x08, 0x14, 0x08, 0x07, 0x04, 0x06, 0x04, 0x04};
byte hum[9] = {0x19, 0x1A, 0x04, 0x08, 0x15, 0x07, 0x05, 0x00};
byte BT[9] = {0x1F, 0x13, 0x15, 0x13, 0x15, 0x13, 0x1F, 0x00};
byte MicOn[9] = {0x11, 0x1B, 0x1F, 0x1F, 0x15, 0x15, 0x11, 0x00};
byte thermo[9] = {0x04, 0x0A, 0x0A, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E};
byte humid[9] = {0x04, 0x04, 0x0A, 0x0A, 0x11, 0x11, 0x11, 0x0E};
byte shield[9] = {0x1F, 0x1F, 0x15, 0x0A, 0x15, 0x1F, 0x0E, 0x04};

int LCDtimmer = 0; int Repeat; int LCDlock = 1;

LiquidCrystal_I2C lcd(0x3F, 16, 2);
RGBLed led(11, 10, 9, COMMON_CATHODE);
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial Serial1(0,1);

int i; char ii; int g = 0; int LastH; int LastF; int Fr; int H;
unsigned long LapCounter; int Xled; int TempHumCounter;
int Q = 0; int q = 0; int Sum; int BL; int LEDlevel = constrain(LEDlevel, 1, 100);
int e = 0; int f = 0; int LEDlap = 0; int Uprompt = 0;
String Huh = "HUH?  I DON'T UNDERSTAND THAT COMMAND!"; 
String Cant = "YOU CAN'T DO THAT!"; int Lastresult = 0;
const char data[] = "what time is it"; int echo = 0;
char red = 0; char green = 0; char blue = 0; int Custom = false;

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    lcd.print(datestring);
}

void SprintDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void LCDscroll() {
  for (int positionCounter = 0; positionCounter < 18; positionCounter++) {
    lcd.scrollDisplayLeft();
    delay(50);
  }
  delay(1000);
  for (int positionCounter = 0; positionCounter < 18; positionCounter++) {
    lcd.scrollDisplayRight();
    delay(50);
  }
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(MicRelay1, OUTPUT);
  pinMode(BtRelay2, OUTPUT);
  pinMode(Reset, OUTPUT);
  pinMode(BTstatus, INPUT);
  pinMode(REswitch, INPUT);
  pinMode(Pair, OUTPUT);
  digitalWrite(MicRelay1, HIGH);
  digitalWrite(BtRelay2,HIGH);
  digitalWrite(Reset, HIGH);
  digitalWrite(Pair,LOW);
  lcd.begin();
  delay(100);
  lcd.clear();
  lcd.backlight();
      Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
  lcd.createChar(1, check);
  lcd.createChar(9, antenna);
  lcd.createChar(4, MicOn);
  lcd.createChar(5, thermo);
  lcd.createChar(6, BT);
  lcd.createChar(7, humid);
  lcd.createChar(8, castle);
  lcd.createChar(10, degF);
  lcd.createChar(11, hum);
  lcd.createChar(0, shield);
  dht.begin();
  lcd.setCursor(22, 0);
  lcd.print("10-29");
  lcd.setCursor(22, 1);
  lcd.print("HENRY");
  LCDscroll();
  delay(1500);
  lcd.clear();
  LastH = 0;
  LastF = 0;
  Fr = 0;
  H = 0;
  TempHumCounter = 0;
  i = 0;
  Xled = 1;
  q = 0;
  Q = 0;
  Sum = 0;
  BL = 1;
  LEDlevel = 1;
  g = 0; 
  LapCounter = 0;
  int REtimmer = 0;
      uint8_t count = sizeof(data);
    uint8_t written = Rtc.SetMemory((const uint8_t*)data, count);
  Serial.print(F("\n\n\n\n\n\n\n\n\n\n\n\n **************************************\n"));
  Serial.print(F("            * WARNING! *\n"));
  Serial.print(F("     USE OF THIS SOFTWARE REQUIRES\n"));
  Serial.print(F("   PRIOR APPROVAL BY THE PROPRIETOR\n\n"));
  Serial.print(F("\t  10-29H aka:'HENRY'\n\n"));
  Serial.print(F("  JAS.519955 Ver:1.51 March,2019\n"));
  Serial.print(F(" **************************************\n"));
  Serial.print(F("\t      Help: '?'\n\n\n"));
  Serial.print(F(" Welcome to 10-29H (later identified as HENRY).\n")); 
  encoder.setPosition(10 / ROTARYSTEPS); // start with the value of 0.
  lcd.home();
  }

void loop() {
  lcd.setCursor(0,1);
  lcd.write(0);
  i = Serial.read();
  if(Uprompt == 0) { Serial.print("\n >> "); Uprompt = 1;}
      RtcDateTime now = Rtc.GetDateTime();
  if(Serial.available() > 0 && Xled == 1) { led.brightness(255, 0, 0, LEDlevel); }
  int REtimmer = millis();
  if(i == 'E') { Serial.print(" - Echo mode ON"); echo = 1; Uprompt = 0;} else if(i == 'e') {
    Serial.print("Echo mode OFF"); echo = 0; Uprompt = 0; } 
    if(echo == 1 && Serial.available() > 0) { Serial.print(" - integer:"); Serial.print(int(i)); Serial.print("  character:");
    Serial.print(char(i)); Serial.print("  HEX:"); Serial.print(i,HEX); Serial.print("'"); Serial.print(49); Serial.print("'"); Serial.print(" - "); Uprompt = 0; }
    uint8_t buff[20];
    const uint8_t count = sizeof(buff);
    uint8_t gotten = Rtc.GetMemory(buff, count);
    encoder.tick();
  int newPos = encoder.getPosition() * ROTARYSTEPS;
  if (newPos < ROTARYMIN) {
    encoder.setPosition(ROTARYMIN / ROTARYSTEPS);
    newPos = ROTARYMIN;
  } else if (newPos > ROTARYMAX) {
    encoder.setPosition(ROTARYMAX / ROTARYSTEPS);
    newPos = ROTARYMAX;
  }
  if (lastPos != newPos) {
    Serial.print("Encoder position:");
    Serial.print(newPos);
    lastPos = newPos;
  }
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);
  Fr = f;
  H = h;
  if (isnan(h) || isnan(f)) {
    lcd.setCursor(7, 1);
    lcd.print("err");
  } else {
//    if (TempHumCounter <= 200) {
      lcd.setCursor(7, 1);
      lcd.write(5);
      lcd.print(Fr);
      lcd.write(10);
      lcd.setCursor(12,1);
      lcd.write(7);
//    } else if (TempHumCounter > 200) {
//      lcd.setCursor(0, 1);
      lcd.print(H);
//      lcd.setCursor(2, 1);
      lcd.write(11);
    } 
//    }
    lcd.home();
    printDateTime(now);
    
/*    Time t = rtc.time();
      int dotw = t.day;
      switch (dotw) {
        case 1: lcd.setCursor(6, 0); lcd.print("Sun"); break;
        case 2: lcd.setCursor(6, 0); lcd.print("Mon"); break;
        case 3: lcd.setCursor(6, 0); lcd.print("Tue"); break;
        case 4: lcd.setCursor(6, 0); lcd.print("Wed"); break;
        case 5: lcd.setCursor(6, 0); lcd.print("Thu"); break;
        case 6: lcd.setCursor(6, 0); lcd.print("Fri"); break;
        case 7: lcd.setCursor(6, 0); lcd.print("Sat"); break;
      }
      */
  if (i == '?') {
    Serial.print(F("\n\t         Options & Commands\n"));
    Serial.print(F("-----------------------------------------------------\n"));
    Serial.print(F("  M .....Mic On\t\t\tm .....Mic Off\n"));
    Serial.print(F("  B .....Bluetooth On\t\tb .....Bluetooth Off\n"));
    Serial.print(F("  L .....LED On\t\t\tl .....LED Off\n"));
    Serial.print(F("  T(t) ..Time & Temp\t\tX(x) ..LCD/LED Off\n"));
    Serial.print(F("  D .....Backlight On\t\td .....Backlight off\n"));
    Serial.print(F("  + .....LED +10% \t\t- .....LED -1% dimmer\n"));
    Serial.print(F("  > .....LED Max bright\t\t< .....LED Min bright\n"));
    Serial.print(F("  # .....Status check\t\t? .....Help Menu\n"));
    Serial.print(F("  @ .....ColorFx(1min)\t\t~ .....Reset\n  R(r) ..Repeat mode\t\tS(s) ..NON-Repeat mode\n"));
    Serial.print(F("  E .....Echo ON\t\te .....Echo OFF\n  * .....All lights ON\n"));
    Serial.print(F("------------------------------------------------------\n"));
    Uprompt = 0;
  }
  if(i == '\'') {Uprompt = 0; Serial.print("Bluetooth pairing mode"); digitalWrite(Pair,HIGH); delay(100); digitalWrite(Pair,LOW); delay(100); digitalWrite(Pair,HIGH); delay(100); digitalWrite(Pair,LOW);}
  if(digitalRead(REswitch) == LOW) { Uprompt = 0; Serial.print(" [SELECT] "); lcd.setCursor(15,0); lcd.write(1); delay(2000); lcd.setCursor(15,0); lcd.print(" "); }
  if (i == 't' || i == 'T') {
    Uprompt = 0;
    Serial.print("\n\t");
    SprintDateTime(now);
    if (isnan(h) || isnan(f)) {
      Serial.println(F("\n Failed to read from DHT sensor!"));
    } else {
      Serial.print(F("\n     Temperature: "));
      Serial.print(Fr);
      Serial.print(F("°F"));
      Serial.print(F("   Humidity: "));
      Serial.print(H);
      Serial.println(" ");
    }
  }
  if (i == 'L') {
    Uprompt = 0;
    Serial.print(F("LED ON"));
    Xled = 1;
    lcd.setCursor(22, 0);
    lcd.print("LED ON");
    LCDscroll();
  }
  if (i == 'l') {
    Uprompt = 0;
    Serial.print(F("LED OFF"));
    Xled = 0;
    lcd.setCursor(22, 0);
    lcd.print("LED OFF");
    LCDscroll();
  }
  if (i == 'M') {
    digitalWrite(MicRelay1, LOW);
    Serial.print(F("Microphone - ON"));
    Uprompt = 0;
  }
  if (i == 'm') {
    digitalWrite(MicRelay1, HIGH);
    Serial.print(F("Microphone - OFF"));
    Uprompt = 0;
  }
  if (i == 'B') {
    digitalWrite(BtRelay2, LOW);
    Serial.print(F("Bluetooth - ON"));
    Uprompt = 0;
  }
  if (i == 'b') {
    digitalWrite(BtRelay2, HIGH);
    Serial.print(F("Bluetooth - OFF"));
    Uprompt = 0;
  }
  if (i == 'x' || i == 'X') {
    Xled = 0;
    Serial.print(F("BLACKOUT!"));
    lcd.setCursor(20, 0);
    lcd.print("BLACKOUT!");
    LCDscroll();
    lcd.noBacklight();
    BL = 0;
    LCDlock = 1;
    Uprompt = 0;
  }
  if (i == '*') {
    lcd.backlight();
    Xled = 1;
    lcd.setCursor(20, 0);
    lcd.print("ILLUMINATED");
    LCDscroll();
    Serial.print(F("ALL LIGHTS ON"));
    BL = 1;
    Uprompt = 0;
  }
 if(i == 's') { LCDlock = 1; Uprompt = 0; Serial.print("SLEEP TIMMER OFF"); }
 if(i == 'S') { LCDlock = 0; Uprompt = 0; Serial.print("LCD SLEEP TIMMER: ON, 1 MIN"); }
  if (i == 'D') {
    Serial.print(F("LCD Backlight ON"));
    lcd.backlight();
    lcd.setCursor(22, 0);
    lcd.print("LCD ON");
    LCDscroll();
    BL = 1;
    Uprompt = 0;
  }
  if (LCDtimmer == 4 && LCDlock == 0) { lcd.noBacklight(); }
  if (i == 'd') {
    Serial.print(F("LCD Backlight OFF"));
    lcd.setCursor(22, 0);
    lcd.print("LCD OFF");
    LCDscroll();
    lcd.noBacklight();
    BL = 0;
    Uprompt = 0;
  }
  if (i == '-') {
    if (LEDlevel <= 1) {
      LEDlevel = 1;
      Serial.print(F("LED brightness MIN\tLevel:"));
      Serial.print(LEDlevel);
      lcd.setCursor(22, 0);
      lcd.print("LED MIN");
      LCDscroll();
      Uprompt = 0;
    } else {
      LEDlevel = (LEDlevel - 1);
      Serial.print(F("LED DOWN (-)\tLevel:"));
      Serial.print(LEDlevel);
      lcd.setCursor(21, 0);
      lcd.print("LED ");
      lcd.setCursor(26, 0);
      lcd.print(LEDlevel);
      lcd.print("%");
      LCDscroll();
      Uprompt = 0;
    }
  }
  if (i == '>' && Xled == 1) {
    LEDlevel = 100;
    Serial.print(F("LED brightness MAX\tLevel:"));
    Serial.print(LEDlevel);
    lcd.setCursor(22, 0);
    lcd.print("LED MAX");
    LCDscroll();
    Uprompt = 0;
  }
  if (i == '<' && Xled == 1) {
    LEDlevel = 1;
    delay(10);
    Serial.print(F("LED brightness MIN\tLevel:"));
    Serial.print(LEDlevel);
                   lcd.setCursor(22, 0);
                   lcd.print("LED MIN");
    LCDscroll();
    Uprompt = 0;
  }
  if (i == '+') {
    if (LEDlevel >= 90) {
      LEDlevel = 100;
      Serial.print(F("LED brightness MAX\tLevel:"));
      Serial.print(LEDlevel);
      lcd.setCursor(22, 0);
      lcd.print("LED MAX");
      LCDscroll();
      Uprompt = 0;
    } else {
      LEDlevel += 10;
      Serial.print(F("LED UP (+)\tLevel:"));
      Serial.print(LEDlevel);
      lcd.setCursor(21, 0);
      lcd.print("LED ");
      lcd.setCursor(26, 0);
      lcd.print(LEDlevel);
      lcd.print("%");
      LCDscroll();
      Uprompt = 0;
    }
  }

  if (i == 'R') {
    Repeat = true;
    Serial.print("Repeat on");
    Uprompt = 0;
  }
  if (i == 'r') {
    Repeat = false;
    g = 0;
    LEDlap = 0;
    Serial.print("Repeat off");
    Uprompt = 0;
  }
  
  if (i == '@') {
      Serial.println("\t\t\tColorFx");
      lcd.setCursor(21, 0);
      lcd.print("ColorFx");
       lcd.noBacklight();
      LCDscroll();
    if(Repeat == false) { g = 1; }
    if(Repeat == true) { g = 2; }
      Serial.print(F("Begin"));
  }
  while(g >= 1) {
    lcd.noBacklight();
    led.fadeIn(75, 255, 255, 5, 250);
    Serial.print(F(".1"));
    if(Serial.read() > 13){ g = 0; Uprompt = 0; break; } else {
      led.fadeOut(180, 15, 250, 5, 250);
      Serial.print(F(".2"));}
      if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
      led.fadeOut(0, 255, 0, 5, 250);
      Serial.print(F(".3"));}
      if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
      led.fadeOut(0, 0, 255, 5, 250);
      Serial.print(F(".4"));}
      if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
      led.fadeIn(30, 75, 106, 5, 250);
      Serial.print(F(".5"));}
      if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
        led.fadeIn(0, 255, 0, 5, 250);
        Serial.print(F(".6"));}
        if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
        if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
        led.fadeIn(0, 0, 255, 5, 250);
        Serial.print(F(".7"));}
        led.fadeOut(255, 0, 255, 5, 250);
        Serial.print(F(".8"));}
        if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
        led.fadeIn(0, 0, 255, 5, 250);
        Serial.print(F(".9"));}
        if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
        led.fadeIn(0, 255, 0, 5, 250);
        Serial.print(F(".10"));}
        if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
          led.fadeOut(255, 255, 0, 5, 250);
          Serial.print(F(".11"));}
          if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
          led.fadeOut(30, 0, 0, 5, 250);
          Serial.print(F(".12"));}
          if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
          led.fadeOut(0, 255, 0, 5, 250);
          Serial.print(F(".13"));}
          if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
          led.fadeOut(0, 0, 255, 5, 250);
          Serial.print(F(".14"));}
          if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
          led.fadeIn(50, 120, 175, 5, 250);
          Serial.print(F(".15")); }
          if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
            led.fadeIn(0, 255, 0, 5, 250);
            Serial.print(F(".16"));}
            if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
            led.fadeIn(0, 0, 255, 5, 250);
            Serial.print(F(".17"));}
            if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
            led.fadeOut(255, 0, 255, 5, 250);
            Serial.print(F(".18"));}
            if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
            led.fadeIn(0, 0, 255, 5, 250);
            Serial.print(F(".19"));}
            if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
            led.fadeIn(0, 255, 0, 5, 250);
            Serial.print(F(".20"));}
            if(Serial.read() > 13) { g = 0; Uprompt = 0; break; } else {
              led.fadeOut(255, 255, 0, 5, 250);}
              if(g == 0) { ; }
              if(g == 1) {
                g = 0;
                Serial.print("  Complete");
                Uprompt = 0;
              }
              if(g == 2) {
                Serial.println("  Repeat");
                LEDlap++;
              }
    }
    if(i == '-') { Xled = 1; }
    if(i == 'C') { Custom = true; red = 0; green = 0; blue = 0;
      Serial.print("Enter RGB led custom values 0-255 (format - rrr,ggg,bbb):"); }
      if(Custom == true) {
  while (Serial.available() > 0) {

    // look for the next valid integer in the incoming serial stream:
    int red = Serial.parseInt();
    // do it again:
    int green = Serial.parseInt();
    // do it again:
    int blue = Serial.parseInt();

    // look for the newline. That's the end of your sentence:
    if (Serial.read() == '\n') {
      // constrain the values to 0 - 255 and invert
      // if you're using a common-cathode LED, just use "constrain(color, 0, 255);"
      red = 255 - constrain(red, 0, 255);
      green = 255 - constrain(green, 0, 255);
      blue = 255 - constrain(blue, 0, 255);

      Serial.print(red, HEX);
      Serial.print(green, HEX);
      Serial.println(blue, HEX);
    }
  } led.brightness(red, green, blue, LEDlevel); Xled = 2; Uprompt = 0; Custom = false;}
    

Sum = Q + i;
if (i == '~') {
    lcd.setCursor(22, 0);
    lcd.print("RESET?");
    LCDscroll();
    Q = 126;
      Sum = i + Q;
    Serial.print(F("\n\n\tCONFIRM RESET?\n\t    Y or N\n"));
    delay(100);
    Uprompt = 0;
}
  if (Sum == 215 || Sum == 247) {
    Serial.print(F("\t!!! SYSTEM RESET !!!"));
    lcd.clear(); lcd.home(); lcd.print("! SYSTEM RESET !"); delay(500);
    lcd.clear(); lcd.setCursor(0, 1); lcd.print("! SYSTEM RESET !"); delay(500);
    lcd.clear(); lcd.print("! SYSTEM RESET !"); delay(500);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("! SYSTEM RESET !");
    delay(1000);
    lcd.clear();
    lcd.noBacklight();
    delay(300);
    digitalWrite(Reset, LOW);
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("  RESET FAILED  ");
    lcd.backlight();
    Q = 0;
    Serial.print("!E!E!   RESET FAILED!    !E!E!");
    Uprompt = 0;
  }
  if (Sum == 204 || Sum == 236) {
    lcd.setCursor(22, 0);
    lcd.print(" Cancel ");
    delay(1500);
    LCDscroll();
    Serial.print(F("\tAction Canceled"));
    Q = 0;
    Uprompt = 0;
  }
  if (i == '#') {
    Serial.print(F("\n\t~~~~~~~~~~~~~~~~~~~~~~~~~~~ CURRENT SYSTEM STATUS ~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\t\t\t\t   "));
    SprintDateTime(now);
    Serial.print(F("\n\tMicrophone\tBluetooth\tLED\t  LED Brightness\tLCD Backlight\n"));
    if (digitalRead(MicRelay1) == LOW) {
    Serial.print(F("\t   ON"));
  } else { Serial.print(F("\t   OFF")); }
  if (digitalRead(BtRelay2) == LOW) { Serial.print(F("\t\t   ON"));
  } else { Serial.print(F("\t\t   OFF")); }
  if (Xled == 1 || Xled == 2) { Serial.print(F("\t\t ON\t       ")); Serial.print(LEDlevel); Serial.print("%");
  } else { Serial.print(F("\t\tOFF\t       ")); Serial.print("0%"); }
  if (BL == 1) { Serial.print(F("\t\t      ON"));
    } else { Serial.print(F("\t\t     OFF")); } Serial.print(F("\n\n\t      LapCount:")); Serial.print(LapCounter);
    if (Repeat == true) { Serial.print("\t   Repeat ON"); } else { Serial.print("\t   Repeat OFF"); }
    if (LCDlock == 0) {Serial.print("\t   Sleep Timmer ON"); } else { Serial.print("\t   Sleep Timmer OFF"); }
    Serial.print("\n\t\t\t Rotary Encoder position:"); Serial.print(newPos);
    Serial.print(F("\n\n\t~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
    Uprompt = 0;
  }
  if (digitalRead(BTstatus) == HIGH) { lcd.setCursor(6,1); lcd.write(9);
  } else { lcd.setCursor(6,1); lcd.print(" "); }
  if (digitalRead(MicRelay1) == LOW && digitalRead(BtRelay2) == LOW) { lcd.setCursor(4,1); lcd.write(4); lcd.setCursor(5,1); lcd.write(6);
    if (Xled == 1) { led.brightness(200, 255, 0, LEDlevel);
    } else if(Xled == 0) { led.off(); } }
    
  if (digitalRead(BtRelay2) == LOW && digitalRead(MicRelay1) == HIGH) { lcd.setCursor(4,1); lcd.print(" "); lcd.setCursor(5,1); lcd.write(6);
    if (Xled == 1) { led.brightness(0, 70, 255, LEDlevel);
    } else if(Xled == 0) { led.off(); } }
  if (digitalRead(MicRelay1) == LOW && digitalRead(BtRelay2) == HIGH) { lcd.setCursor(4,1); lcd.write(4); lcd.setCursor(5,1); lcd.print(" ");
  if (Xled == 1) { led.brightness(66, 235, 244, LEDlevel); delay(500);
      } else if(Xled == 0) { led.off(); } }
    
      if (digitalRead(MicRelay1) == HIGH && digitalRead(BtRelay2) == HIGH) { lcd.setCursor(4,1); lcd.print("  ");
    if (Xled == 1) { led.brightness(0, 0, 0, LEDlevel);
    } else if(Xled == 0) { led.off(); } }
  
  if (Xled == 0) {
    led.off();
  }
  LastH = H;
  LastF = Fr;
  if (TempHumCounter >= 400) {
    TempHumCounter = 0;
    LCDtimmer++;
  } else {
    TempHumCounter++;
  }
  if (LEDlevel > 100) {
    LEDlevel = 100;
  }
  if (LEDlevel < 1) {
    LEDlevel = 1;
  }
  lcd.setCursor(16, 0);
  lcd.print("               ");
  lcd.setCursor(16, 1);
  lcd.print("               ");
  LapCounter++;

if(i > 48 && Uprompt == 1) {Uprompt = 0; 
  if(i >= 'a') { Serial.print(Huh); lcd.setCursor(0,0); lcd.print("?Err"); }
  if(i < 'a') { Serial.print(Cant); lcd.setCursor(0,0); lcd.print("!Err");} delay(2000); lcd.setCursor(0,0); lcd.print("    ");}
}
