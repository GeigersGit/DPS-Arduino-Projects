//Temperature/Humidity IP DCP(x) RTU
//Brian Geiger
//10/10/2017
#define VERSION 0.1

#include <LiquidCrystal.h>
#include <SimpleDHT.h>
#include <DS3231_Simple.h>
//#include <EtherCard.h>
//#include <IPAddress.h>

//OS Globals
////////////////////////////////////////////////////////////////////
byte newPage = 1;
byte mode = 0;
#define REFRESH_RATE 2500
unsigned long nextRefresh = 2500;
#define DISPLAY_MODE 0
#define ETHERNET_MODE 1
#define DCP_MODE 2

//LCD Globals
////////////////////////////////////////////////////////////////////
#define rs 7
#define en 6
#define d4 5
#define d5 4
#define d6 3
#define d7 2
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//DHT Globals
////////////////////////////////////////////////////////////////////
SimpleDHT22 tempSensor1;
#define DHT1_PIN 8
SimpleDHT22 tempSensor2;
#define DHT2_PIN 9
float curIntTemp = 0;
float curExtTemp = 0;
#define DHT_SAMPLE_RATE 2500
unsigned long nextDHTSample = DHT_SAMPLE_RATE;

//5-Key Globals
////////////////////////////////////////////////////////////////////
byte keyPressed = 0;
byte heldDown = 0;
unsigned long nextAnalogSample = 0;
#define READ_DELAY 200
#define KEYPAD_ANALOG_PIN 1
#define BTN_UP 0
#define LEFT   1
#define PLUS   2
#define MINUS  3
#define RIGHT  4
#define SELECT 5
#define A_DEV    20         // allowed deviation from input values
#define A_LEFT   20         // input from SW1
#define A_PLUS   150        // input from SW2
#define A_MINUS  330        // input from SW3
#define A_RIGHT  500        // input from SW4
#define A_SELECT 730        // input from SW5
#define A_MAX    1023-A_DEV // max analog input

//RTC Globals
////////////////////////////////////////////////////////////////////
DS3231_Simple Clock;





void mainLoop(){
  switch(mode){
////////////////////////////////////////////////////////////////////
    case DISPLAY_MODE:
////////////////////////////////////////////////////////////////////
      if(newPage){
        lcd.clear();
        lcd.print("Temp Display");
        delay(1000);
        lcd.clear();
        lcd.print("Int Temp: "); lcd.print(curIntTemp);
        lcd.setCursor(0,1);
        lcd.print("Ext Temp: 78.9 F");
        newPage = 0;
      }
      if(keyPressed == PLUS){
        mode = DCP_MODE;
        keyPressed = 0;
        newPage = 1;
      }
      if(keyPressed == MINUS){
        mode = ETHERNET_MODE;
        keyPressed = 0;
        newPage = 1;
      }
      if(millis() > nextRefresh){
        lcd.clear();
        lcd.print("Int Temp: "); lcd.print(curIntTemp);
        lcd.setCursor(0,1);
        lcd.print("Ext Temp: "); lcd.print(curExtTemp);
        nextRefresh = millis() + REFRESH_RATE;
      }
    break;
////////////////////////////////////////////////////////////////////
    case ETHERNET_MODE:
////////////////////////////////////////////////////////////////////
      if(newPage){
        lcd.clear();
        lcd.print("Ethernet Menu");
        delay(1000);
        lcd.clear();
        lcd.print("IP:   10.0.3.20");
        lcd.setCursor(0,1);
        lcd.print("Gate: 10.0.0.254");
        newPage = 0;
      }
      if(keyPressed == PLUS){
        mode = DISPLAY_MODE;
        keyPressed = 0;
        newPage = 1;
      }
      if(keyPressed == MINUS){
        mode = DCP_MODE;
        keyPressed = 0;
        newPage = 1;
      }
    break;
////////////////////////////////////////////////////////////////////
    case DCP_MODE:
////////////////////////////////////////////////////////////////////
      if(newPage){
        lcd.clear();
        lcd.print("DCP Menu");
        delay(1000);
        lcd.clear();
        lcd.print("Packets RX'd: 0");
        lcd.setCursor(0,1);
        lcd.print("Packets TX'd: 0");
        newPage = 0;
      }
      if(keyPressed == PLUS){
        mode = ETHERNET_MODE;
        keyPressed = 0;
        newPage = 1;
      }
      if(keyPressed == MINUS){
        mode = DISPLAY_MODE;
        keyPressed = 0;
        newPage = 1;
      } 
    break;
  } 
}

////////////////////////////////////////////////////////////////////
void checkTempSensors(){
////////////////////////////////////////////////////////////////////
  if(millis() > nextDHTSample){
    float hum1;
    float hum2;
    
    tempSensor1.read2(DHT1_PIN, &curIntTemp, &hum1, NULL);
    curIntTemp = ((curIntTemp * 9/5) + 32); //Fahrenheit conversion
    
    //tempSensor2.read2(DHT2_PIN, &curExtTemp, &hum2, NULL);
    //curExtTemp = ((curExtTemp * 9/5) + 32); //Fahrenheit conversion
    curExtTemp = curIntTemp;

    nextDHTSample = millis() + DHT_SAMPLE_RATE; 
  }
}

////////////////////////////////////////////////////////////////////
void checkKeypad() {
////////////////////////////////////////////////////////////////////
  //only take readings every 20ms
  if(millis() < nextAnalogSample)
    return;

  //take voltage reading and assign it a button number
  int analogValue = analogRead(KEYPAD_ANALOG_PIN);
  if (analogValue >= A_LEFT - A_DEV && analogValue <= A_LEFT + A_DEV) keyPressed = LEFT;
  else if (analogValue >= A_PLUS - A_DEV && analogValue <= A_PLUS + A_DEV) keyPressed = PLUS;
  else if (analogValue >= A_MINUS - A_DEV && analogValue <= A_MINUS + A_DEV) keyPressed = MINUS;
  else if (analogValue >= A_RIGHT - A_DEV && analogValue <= A_RIGHT + A_DEV) keyPressed = RIGHT;
  else if (analogValue >= A_SELECT - A_DEV && analogValue <= A_SELECT + A_DEV) keyPressed = SELECT;
  else keyPressed = BTN_UP;

  nextAnalogSample = millis() + READ_DELAY;

  if(keyPressed == BTN_UP){
    heldDown = 0;
    return;
  }
  heldDown = 1;
}

////////////////////////////////////////////////////////////////////
void printDate(byte mo, byte d, byte y, byte h, byte m){
////////////////////////////////////////////////////////////////////
  //Prints out the date
  
  lcd.print(mo); lcd.print("/");
  lcd.print(d); lcd.print("/");
  //lcd.print("20"); 
  lcd.print(y); lcd.print(" ");
  if(h >= 12){
    lcd.print(h - 12); lcd.print(":");
    if(m >= 10)
      lcd.print(m);
    else{
      lcd.print("0"); lcd.print(m);
    }
    lcd.println("PM");
  }
  else{
    lcd.print(" "); lcd.print(h); lcd.print(":");
    if(m >= 10)
      lcd.print(m);
    else{
      lcd.print("0"); lcd.print(m);
    }
    lcd.println("AM");
  }
}

////////////////////////////////////////////////////////////////////
void setup() {
////////////////////////////////////////////////////////////////////
  Serial.begin(9600);
  
  //LCD setup
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("Box OS v"); lcd.print(VERSION);

  //RTC setup
    Clock.begin();
    lcd.setCursor(0,1);
    //Print date and time
    DateTime MyDateAndTime = Clock.read();
    printDate(MyDateAndTime.Month,MyDateAndTime.Day,MyDateAndTime.Year,MyDateAndTime.Hour,MyDateAndTime.Minute);
  
  delay(5000);
}

////////////////////////////////////////////////////////////////////
void loop() {
////////////////////////////////////////////////////////////////////
  mainLoop();
  
  checkTempSensors();
  
  checkKeypad();
}
