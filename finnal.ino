

#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>


const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static bool condition[50];
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 8, en = 9, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int chipSelect = 10;

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(6, 7);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif
tmElements_t tm;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

bool getDate(const char *str);
bool getTime(const char *str);
void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
   bool parse=false;
  bool config=false;
  
  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }

  delay(200);
  if (parse && config) {
    Serial.print(F("DS1307 configured Time="));
    Serial.print(__TIME__);
    Serial.print(F(", Date="));
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println(F("DS1307 Communication Error :-{"));
    Serial.println(F("Please check your circuitry"));
  } else {
    Serial.print(F("Could not parse info from the compiler, Time=\""));
    Serial.print(__TIME__);
    Serial.print(F("\", Date=\""));
    Serial.print(__DATE__);
    Serial.println("\"");
  }

  Serial.println(F("\n\nAdafruit finger detect test"));
  lcd.clear();
  delay(10);
  lcd.print("      press!");

  //init SD card
   Serial.print(F("Initializing SD card..."));
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Card failed, or not present"));
    // don't do anything more:
    while (1);
  }
  Serial.println(F("card initialized."));
  record(-2);
  
  // Set the data rate for the sensor serial port
  // Begin serial communication with the fingerprint sensor
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println(F("Found fingerprint sensor!"));
  } else {
    Serial.println(F("Did not find fingerprint sensor :("));
    lcd.clear();
    lcd.print("sensor lose!");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print(F("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example."));
  }
  else {
    Serial.println(F("Waiting for valid finger..."));
      Serial.print(F("Sensor contains ")); Serial.print(finger.templateCount); Serial.println(F(" templates"));
  }
}

void loop()                     // run over and over again
{
  getFingerprintID();   //run
  delay(100);            //don't ned to run this at full speed.//may we delay(500)
  lcd.clear();
  delay(5);
  lcd.print("     press!");
}


uint8_t getFingerprintID() {
 
    
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image taken"));
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(F("No finger detected"));
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println(F("Imaging error"));
      return p;
    default:
      Serial.println(F("Unknown error"));
      return p;
  }

  // OK success!

  p = finger.image2Tz();//attempt to extract features from the image
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println(F("Image converted"));
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println(F("Image too messy"));
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println(F("Communication error"));
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println(F("Could not find fingerprint features"));
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println(F("Could not find fingerprint features"));
      return p;
    default:
      Serial.println(F("Unknown error"));
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println(F("Communication error"));
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println(F("Did not find a match"));
    lcd.clear();
    lcd.print(F("       NO!"));
    delay(1000);
    record(-1);
    return p;
  } else {
    Serial.println(F("Unknown error"));
    lcd.clear();
    return p;
  }

  // found a match!
  Serial.print(F("Found ID #")); Serial.print(finger.fingerID);
  Serial.print(F(" with confidence of ")); Serial.println(finger.confidence);
  Serial.println(printName(finger.fingerID));
  lcd.clear();
  if(!condition[finger.fingerID]){
    lcd.print(F("Weclcome"));
  }
  else {
    lcd.print(F("GoodBye"));
  }
  lcd.print(F("   ID #"));lcd.print(finger.fingerID);
  lcd.setCursor(0,1);
 
  lcd.print(printName(finger.fingerID));

  record(finger.fingerID);
  delay(1000);
  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
//  Serial.print("dezFound ID #"); Serial.print(finger.fingerID);
//  Serial.print(" with confidence of "); Serial.println(finger.confidence);
//  delay(1000);
  return finger.fingerID;
}
void record(int id){
    // if the file is available, write to it:
    File dataFile = SD.open("505.txt", FILE_WRITE);
    tmElements_t tm;
    if(RTC.read(tm)){
    if (dataFile) {
    String t = "";
    t+=(String)tmYearToCalendar(tm.Year);
    t+=".";
    t+=(String)tm.Month;
    t+=".";
    t+=(String)tm.Day;
    t+=",";
    t+=(String)tm.Hour;
    t+=":";
    t+=(String)tm.Minute;
    t+=":";
    t+=(String)tm.Second; 
    t+=",";
   
      if(id>=0){
        if(!condition[id]){
          t+="in";
          condition[id]=true;
        }
        else {
          t+="out";
          condition[id]=false;
        }
      }
      else {
        t+="null";
      }
    
    t+=",";
    t+=id;
    t+=",";
    t+=printName(id);
    
    dataFile.println(t);
    delay(100);
    dataFile.close();
    
    Serial.println(t);
    delay(10);
    
    // size > 3GB
    if(dataFile.size() > 3221225472){
      lcd.clear();
      delay(5);
      lcd.print(F("Warning!"));
      lcd.setCursor(0,1);
      lcd.print(F("SD FULL!"));
      delay(3000);
    }
    //size > 3.3GB
    if(dataFile.size() > 3543348019){
      lcd.clear();
      delay(5);
      lcd.print(F("Error!"));
      lcd.setCursor(0,1);
      lcd.print(F("SD FULL!"));
      while(1)delay(1);
    }
    
    }
    
    else {
    Serial.println(F("error opening datalog.txt"));
    lcd.setCursor(0,1);
    lcd.print(F("    Error SD"));
    while(1);
    }
    }
    else{
       Serial.println(F("DS1307RTC read error"));
       lcd.clear();
       lcd.print(F("DS1307RTC Error"));
    }
} 

bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}



String printName(int i){
  switch(i){
    case -2: return F("Device open");
    case -1:return F("stronger");
    case 0: return F("pika");
    case 1: return F("jam");
    case 3: return F("bob");
    case 4: return F("tom");
    default: return F("null");
  }
}
