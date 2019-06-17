//Arduino Interpreter
//Brian Geiger
//9/19/2017

//#define dhtON 1
#include <EEPROM.h>

#ifdef dhtON
  #include <SimpleDHT.h>
  #include <DS3231_Simple.h>
#endif

#include <EtherCard.h>
//#include <IPAddress.h>
#include <FastLED.h>
#include <LiquidCrystal.h>

#define VERSION 0.14

#define tON 1 //0N
#define tOF 2 //OF
#define tSL 3 //SLOW
#define tME 4 //MEDIUM
#define tFA 5 //FAST
#define tBL 6 //BLINK
#define tMO 7 //MOTOR
#define tLE 8 //LED
#define tSP 9 //SPEED
#define tRE 10 //RED
#define tOR 11 //ORANGE
#define tYE 12 //YELLOW
#define tGR 13 //GREEN
#define tBL 14 //BLUE
#define tPU 15 //PURPLE
#define tAD 16 //ADD
#define tVE 17 //VERSION
#define tST 18 //STATUS
#define tTE 19 //TEMP
#define tHU 20 //HUMIDITY
#define tHI 21 //HIGH
#define tLO 22 //LOW
#define tQU 23 //QUESTION MARK
#define tER 24 //ERASE EEPROM HISTORY LOG
#define tPR 25 //PRINT EEPROM HISTORY LOG
#define tPA 26 //PARTY
#define tERR 0x7E //PARSE ERROR
#define tEOL 0x7F //END OF LINE 

byte TOKS[] = {
  'o','n',tON, //ON
  'o','f',tOF, //OFF
  'f','\0',tOF, //OFF 
  's','l',tSL, //SLOW
  'm','e',tME, //MEDIUM
  'f','a',tFA, //FAST
  'b','l',tBL, //BLINK
  'm','o',tMO, //MOTOR
  'l','e',tLE, //LED
  's','p',tSP, //SPEED
  'r','\0',tRE, //RED
  'o','\0',tOR, //ORANGE
  'y','\0',tYE, //YELLOW
  'g','\0',tGR, //GREEN
  'b','\0',tBL, //BLUE
  'p','\0',tPU, //PURPLE
  'a','d',tAD, //ADD
  'v','e',tVE, //VERSION
  's','t',tST, //STATUS
  't','e',tTE, //TEMP
  'h','u',tHU, //HUMIDITY
  'h','i',tHI, //HIGH
  'l','o',tLO, //LOW
  '?','\0',tQU, //QUESTION 
  'p','a',tPA, //PARTY
  'e','r',tER, //ERASE EEPROM HISTORY LOG
  'p','r',tPR, //PRINT EEPROM HISTORY LOG
  '\0'
};

//PARSING

char inputBuffer[20] = {'\0'};
byte tokenBuffer[10] = {'\0'};
byte tokenStart = 0;
byte tokenEnd = 0;
byte currentToken = 0;
byte phase = 0;
byte nextByte;
int i = 0;

//LED

#define BOARD_LED_PIN 13 
#define EXT_LED_PIN_CATHODE 3 //RED IF HIGH
#define EXT_LED_PIN_ANODE 4  //GREEN IF HIGH
byte ledCycle = 0b11111111;
unsigned long ledTimeStamp = 1000;
int ledSpeed = 1000;

#ifdef dhtON
  //DHT & EEPROM
  
  SimpleDHT22 dht22;
  #define DHT_PIN 8 //Temperature and Humidity Sensor
  #define EEPROM_LATEST_ENTRY_ADDR  1023 //Last two bytes of EEPROM store curent
  #define MAX_ENTRY_ADDR 672 //96 x 7 bytes per entry
  #define NEXT_ENTRY 7
  unsigned long fiveSecondDHTTimeStamp = 10000;
  unsigned long fifteenMinuteDHTTimeStamp = 900000;
  byte curTemperature = 0;
  byte curHumidity = 0;
  byte tempf;
  int curEepromAddr = 0;
  byte temp_min_max = 0;
  int tempAddr = 0;
  byte count;
  
  //RTC
  
  DS3231_Simple Clock;
  DateTime MyDateAndTime;
  byte _month;
  byte _day;
  byte _year;
  byte _hour;
  byte _minute;
#endif

//UDP

static byte myip[] = { 10,0,3,20 };// ethernet interface ip address
static byte gwip[] = { 10,0,0,254 };// gateway ip address
static byte mymac[] = { 0x70,0x69,0x69,0x2D,0x30,0x31 };
byte Ethernet::buffer[65]; // tcp/ip send and receive buffer

//NeoPixel LED

#define NUM_LEDS 5
#define LED_STRIP_PIN 9
CRGB leds[NUM_LEDS];

//RAINBOW EFFECT

byte h = 0;
//unsigned long rainbowTimeStamp = 8000;

//LCD SCREEN

#define rs 7
#define en 6
#define d4 5
#define d5 4
#define d6 3
#define d7 2
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
unsigned long lcdTimeStamp = 500;
byte lcdbool = 0;
char string[] = "PARTY BOX       ";
char tempchar = 0;

void partyStateMachine(){
  
  if(millis() > lcdTimeStamp){

    lcd.setCursor(0, 1);
    lcd.print(string);

    //lcd.scrollDisplayLeft();
    
    if(!lcdbool){
      
      strcpy(string,"PARTY  |(o _ o)|");
      lcdbool = 1;  
    }
    else{
      
      strcpy(string, "BOX    /(o _ o)/");
      lcdbool = 0;
    }
   
    leds[0] = CHSV(h,255,255);
    leds[1] = CHSV(h,255,255);
    leds[2] = CHSV(h,255,255);
    leds[3] = CHSV(h,255,255);
    leds[4] = CHSV(h,255,255);
    FastLED.show();
    lcdTimeStamp = millis() + 500;
    h += 30;
  }
}


void rainbowEffect(){
  //Make LED strip do rainbow effect
  /*
  if(millis() > rainbowTimeStamp){
    for(i = 0; i < NUM_LEDS; i++){
      leds[i] = CHSV(h + (i* 10),255,255);
    }
    FastLED.show(); 
    //FastLED.delay(1000/120); //120 FPS
    h++; 
  rainbowTimeStamp = millis() + 8000;
  }
  */
}

void udpSerialPrint(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len){
  //Listen for commands from UDP
  //IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);
  
  Serial.println("UDP Packet Recieved");
  Serial.print("Destination Port: ");
  Serial.println(dest_port);
  Serial.print("Source Port: ");
  Serial.println(src_port);
  Serial.print("Source IP: ");
  ether.printIp(src_ip);
  Serial.println();
  Serial.print("data: ");
  Serial.println(data);
  Serial.println();

  //Add to input buffer and Parse
  strcpy(inputBuffer, data);
  Serial.println(inputBuffer);
  phase = 1;
}

void dhtStateMachine(){
  #ifdef dhtON
  //Update current temp every five seconds
  //Update logged temp every 15 minutes
  
  if(millis() > fiveSecondDHTTimeStamp){
    dht22.read(DHT_PIN, &curTemperature, &curHumidity, NULL); 
    curTemperature = ((curTemperature * 9/5) + 32); //Fahrenheit conversion
    fiveSecondDHTTimeStamp = millis() + 5000;
  } 
  if(millis() > fifteenMinuteDHTTimeStamp){
    
    MyDateAndTime = Clock.read();
    
    EEPROM.write(curEepromAddr, curTemperature);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, curHumidity);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, MyDateAndTime.Month);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, MyDateAndTime.Day);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, MyDateAndTime.Year);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, MyDateAndTime.Hour);
    curEepromAddr += 1;
    EEPROM.write(curEepromAddr, MyDateAndTime.Minute);
    curEepromAddr += 1;

    if(curEepromAddr == MAX_ENTRY_ADDR) {curEepromAddr = 0;}
    EEPROM.put(EEPROM_LATEST_ENTRY_ADDR, curEepromAddr);

    Serial.println("Wrote to EEPROM");
    fifteenMinuteDHTTimeStamp = millis() + 900000;
  }
  #endif
}

void ledStateMachine(){
  //Last two bites of ledCycle byte drive output pins
  //Byte is rotated left twice every 1000ms or as dictated by led blink speed
  
  if(millis() > ledTimeStamp){
    (ledCycle & (1 << 7)) ? (digitalWrite(EXT_LED_PIN_CATHODE, HIGH)) : (digitalWrite(EXT_LED_PIN_CATHODE, LOW));
    (ledCycle & (1 << 6)) ? (digitalWrite(EXT_LED_PIN_ANODE, HIGH)) : (digitalWrite(EXT_LED_PIN_ANODE, LOW));
    if(!(ledCycle & (1 << 7)) != !(ledCycle & (1 << 6)))
      (digitalWrite(BOARD_LED_PIN, HIGH));
    else
      (digitalWrite(BOARD_LED_PIN, LOW));

    if(!(ledCycle & (1<<7)) && !(ledCycle & (1<<6))) {Serial.println("WARNING BOTH LOW");}

    ledTimeStamp = millis() + ledSpeed;
    ledCycle = rotateByteLeft(rotateByteLeft(ledCycle));
  }
}

void getKeyboardInput(){
  //Collect keyboard input, lowercase any uppercase letters and assign chars one at a time to the input buffer

  if(Serial.available() > 0) {
    nextByte = Serial.read();
    if(nextByte == '\r') {
      lcd.setCursor(0, 0);
      lcd.clear();
      lcd.print(inputBuffer);
      inputBuffer[i + 1] = '\0';
      Serial.println(inputBuffer);
      Serial.println();
      phase++;
    }
    else{
      if(nextByte >= 'A' && nextByte <= 'Z') {nextByte += 32;}
      inputBuffer[i] = nextByte;
      i++;
    }
  }
}

void parseTokens(){
  //get start and ending locations of tokens, verify them, and add their token IDs to the token buffer

  tokenStart = 0;
  tokenEnd = 0;
  while(true) {
    
    while(inputBuffer[tokenStart] == ' ') {
      tokenStart++;
    }
    tokenEnd = tokenStart;
    while(inputBuffer[tokenEnd] != ' ' && inputBuffer[tokenEnd] != '\r' && inputBuffer[tokenEnd] != '\0') {
      tokenEnd++;
    }
    tokenEnd--;
 
    tokenize(tokenStart,tokenEnd,inputBuffer);
      
    //Set both indecies to just after word
    tokenEnd++;
    tokenStart = tokenEnd;
    
    //Append EOF token and exit at the end of the buffer
    if(inputBuffer[tokenEnd] == '\0' || inputBuffer[tokenEnd] == '\r'){
      Serial.print("<EOL> "); Serial.println(tEOL); Serial.println();
      tokenBuffer[currentToken] = tEOL;
      currentToken++;
      break;
    }
  }
  phase++;
}

void executeCommand(){
  //Match list of tokens with command patterns and execute any that match
  
  //Check for parse errors
  for(i = 0; tokenBuffer[i] != tEOL; i++){
    if(tokenBuffer[i] == tERR){
      Serial.println("Command not recognized");
      reset();
      return;
    }
  }
  i = 0;

  //LED COMMANDS
  if(tokenBuffer[0] == tLE){
    
    //OFF
    if(tokenBuffer[1] == tOF && tokenBuffer[2] == tEOL){
        
      Serial.println("LED is now OFF");
      ledSpeed = 1000;
      ledCycle = 0b11111111;
      digitalWrite(EXT_LED_PIN_CATHODE, HIGH);
      digitalWrite(EXT_LED_PIN_ANODE, HIGH);
    }
    
    //SOLID GREEN
    else if((tokenBuffer[1] == tGR && tokenBuffer[2] == tEOL) ||
            (tokenBuffer[1] == tON && tokenBuffer[2] == tGR && tokenBuffer[3] == tEOL)){
        
      Serial.println("LED is now GREEN");
      ledSpeed = 1000;
      ledCycle = 0b01010101;
      digitalWrite(EXT_LED_PIN_CATHODE, LOW);
      digitalWrite(EXT_LED_PIN_ANODE, HIGH);   
    }
    
    //SOLID YELLOW
    else if((tokenBuffer[1] == tYE && tokenBuffer[2] == tEOL) ||
            (tokenBuffer[1] == tON && tokenBuffer[2] == tYE && tokenBuffer[3] == tEOL)){
        
      Serial.println("LED is now YELLOW");
      ledSpeed = 1;
      ledCycle = 0b01100110;  
    }
    
    //SOLID RED
    else if((tokenBuffer[1] == tON && tokenBuffer[2] == tEOL) ||
            (tokenBuffer[1] == tRE && tokenBuffer[2] == tEOL) ||
            (tokenBuffer[1] == tON && tokenBuffer[2] == tRE && tokenBuffer[3] == tEOL)){
        
      Serial.println("LED is now RED");
      ledSpeed = 1000;
      ledCycle = 0b10101010;
      digitalWrite(EXT_LED_PIN_CATHODE, HIGH);
      digitalWrite(EXT_LED_PIN_ANODE, LOW);
    }
    
    //BLINKING GREEN
    else if((tokenBuffer[1] == tBL && tokenBuffer[2] == tGR && tokenBuffer[3] == tEOL)){
        
      Serial.println("LED is now BLINKING GREEN");
      ledSpeed = 1000;
      ledCycle = 0b01110111;
      digitalWrite(EXT_LED_PIN_CATHODE, LOW);
      digitalWrite(EXT_LED_PIN_ANODE, HIGH);
    }
    
    //BLINKING RED
    else if((tokenBuffer[1] == tBL && tokenBuffer[2] == tEOL) ||
            (tokenBuffer[1] == tBL && tokenBuffer[2] == tRE && tokenBuffer[3] == tEOL)){
        
      Serial.println("LED is now BLINKING RED");
      ledSpeed = 1000;
      ledCycle = 0b10111011;  
      digitalWrite(EXT_LED_PIN_CATHODE, HIGH);
      digitalWrite(EXT_LED_PIN_ANODE, LOW);
    }
    
    //STATUS
    else if(tokenBuffer[1] == tST){
      
      //Return if tokens 2 - 5 aren't red, green, or off
      for(i = 2; i < 6; i++){
        if(tokenBuffer[6] != tEOL || 
        !(tokenBuffer[i] == tRE || tokenBuffer[i] == tGR || tokenBuffer[i] == tOF)){
          Serial.println("Command not recognized");
          reset();
          return;
        }
      }
      //Read tokens 2, 3, 4, and 5 and assign them to 2 bits each based on color
      Serial.print("LED Status:");
      ledCycle = 0;
      for(i = 2; i < 6; i++){
        if(tokenBuffer[i] == tOF){
          Serial.print(" Off");
          ledCycle |= (1 << (9 - i - (i-2)));
          ledCycle |= (1 << (8 - i - (i-2)));
        }
        else if(tokenBuffer[i] == tRE){
          Serial.print(" Red");
          ledCycle |= (1 << (9 - i - (i-2)));
          
        }
        else if(tokenBuffer[i] == tGR){
          Serial.print(" Green");
          ledCycle |= (1 << (8 - i - (i-2)));
        }
      }
      Serial.println();
      ledSpeed = 1000;
      i = 0;
    }
    
    //BLINK SPEED
    else if(tokenBuffer[1] == tSP){
      
      //SLOW BLINK SPEED
      if(tokenBuffer[2] == tSL && tokenBuffer[3] == tEOL){
        ledSpeed = 1000;
        Serial.print("LED BLINKING SPEED: ");
        Serial.print(ledSpeed);
        Serial.println("ms");
      }
      
      //MEDIUM BLINK SPEED
      else if(tokenBuffer[2] == tME && tokenBuffer[3] == tEOL){
        ledSpeed = 500;
        Serial.print("LED BLINKING SPEED: ");
        Serial.print(ledSpeed);
        Serial.println("ms");
      }
      
      //FAST BLINK SPEED
      else if(tokenBuffer[2] == tFA && tokenBuffer[3] == tEOL){
        ledSpeed = 100;
        Serial.print("LED BLINKING SPEED: ");
        Serial.print(ledSpeed);
        Serial.println("ms");
      }
      
      //VARIABLE BLINK SPEED
      else if(tokenBuffer[2] > 127 && tokenBuffer[3] == tEOL){
        ledSpeed = ((tokenBuffer[i + 2] - 128) * 10);
        Serial.print("LED BLINKING SPEED: ");
        Serial.print(ledSpeed);
        Serial.println("ms");
      }
      else
        Serial.println("Command not recognized");
    }
    
    //LED STRIP COMMANDS
    
    //SET INDIVIDUAL COLORS
    else if(tokenBuffer[1] > 127){
      
      if(tokenBuffer[2] == tOF){
        leds[tokenBuffer[1] - 128] = CRGB(0,0,0);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now off.");
      }
      else if(tokenBuffer[2] == tRE){
        leds[tokenBuffer[1] - 128] = CRGB(255,0,0);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now red.");
      }
      else if(tokenBuffer[2] == tOR && tokenBuffer[3] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB(255,100,0);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now orange.");
      }
      else if(tokenBuffer[2] == tYE && tokenBuffer[3] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB(250,250,0);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now yellow.");
      }
      else if(tokenBuffer[2] == tGR && tokenBuffer[3] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB(0,255,0);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now green.");
      }
      else if(tokenBuffer[2] == tBL && tokenBuffer[3] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB(0,190,255);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now blue.");
      }
      else if(tokenBuffer[2] == tPU && tokenBuffer[3] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB(150,0,255);
        FastLED.show();
        Serial.print("LED ");Serial.print(tokenBuffer[1] - 128);Serial.println(" is now purple.");
      }
      else if(tokenBuffer[2] > 127 && tokenBuffer[3] > 127 && tokenBuffer[4] > 127 && tokenBuffer[5] == tEOL){
        leds[tokenBuffer[1] - 128] = CRGB((tokenBuffer[2] - 128), (tokenBuffer[3] - 128), (tokenBuffer[4] - 128));
        FastLED.show();
        Serial.print("Changed LED ");Serial.print(tokenBuffer[1] - 128);Serial.print(" to color: ");
        Serial.print(tokenBuffer[2] - 128);Serial.print(", ");Serial.print(tokenBuffer[3] - 128);
        Serial.print(", ");Serial.println(tokenBuffer[4] - 128);
      }
      else
        Serial.println("Command not recognized");
    }
    else
      Serial.println("Command not recognized");
  }

 #ifdef dhtON
  //TEMPERATURE COMMANDS
  else if(tokenBuffer[0] == tTE){
    
    //CURRENT TEMPERATURE
    if(tokenBuffer[1] == tQU && tokenBuffer[2] == tEOL){
      Serial.print("Current Temperature is: ");
      Serial.print(curTemperature); Serial.println(" °F");
    }
    
    //HIGHEST TEMPERATURE
    else if(tokenBuffer[1] == tHI && tokenBuffer[2] == tEOL){
      temp_min_max = 0;
      tempAddr = 0;
      for(i = 0; i < MAX_ENTRY_ADDR; i += NEXT_ENTRY){
        tempf = EEPROM.read(i);
        if(tempf >= temp_min_max){
          temp_min_max = tempf;
          tempAddr = i;
        }
      }
      Serial.print("The highest recorded temperature is: ");
      Serial.print(temp_min_max); Serial.println(" °F");
      _month = EEPROM.read(tempAddr + 2);
      _day = EEPROM.read(tempAddr + 3);
      _year = EEPROM.read(tempAddr + 4);
      _hour = EEPROM.read(tempAddr + 5);
      _minute = EEPROM.read(tempAddr + 6);
      Serial.print("Recorded: ");
      printDate(_month,_day,_year,_hour,_minute);
    }
    
    //LOWEST TEMPERATURE
    else if(tokenBuffer[1] == tLO && tokenBuffer[2] == tEOL){
      temp_min_max = 250;
      tempAddr = 0;
      for(i = 0; i < MAX_ENTRY_ADDR; i += NEXT_ENTRY){
        tempf = EEPROM.read(i);
        if(tempf <= temp_min_max && tempf > 0){
          temp_min_max = tempf;
          tempAddr = i;
        }
      }
      Serial.print("The lowest recorded temperature is: ");
      Serial.print(temp_min_max); Serial.println(" °F");
      _month = EEPROM.read(tempAddr + 2);
      _day = EEPROM.read(tempAddr + 3);
      _year = EEPROM.read(tempAddr + 4);
      _hour = EEPROM.read(tempAddr + 5);
      _minute = EEPROM.read(tempAddr + 6);
      Serial.print("Recorded: ");
      printDate(_month,_day,_year,_hour,_minute);
    }
    else
      Serial.println("Command not recognized");
  }
  
  //HUMIDITY COMMANDS
  else if(tokenBuffer[0] == tHU){
    
    //CURRENT HUMIDITY
    if(tokenBuffer[1] == tQU && tokenBuffer[2] == tEOL){
      Serial.print("Current Humidity is: ");
      Serial.print(curHumidity); Serial.println("%");
    }
    
    //HIGHEST HUMIDITY
    else if(tokenBuffer[1] == tHI && tokenBuffer[2] == tEOL){
      temp_min_max = 0;
      tempAddr = 0;
      for(i = 0; i < MAX_ENTRY_ADDR; i += NEXT_ENTRY){
        tempf = EEPROM.read(i + 1);
        if(tempf >= temp_min_max){
          temp_min_max = tempf;
          tempAddr = i;
        }
      }
      Serial.print("The highest recorded humidity is: ");
      Serial.print(temp_min_max); Serial.println(" %");
      _month = EEPROM.read(tempAddr + 2);
      _day = EEPROM.read(tempAddr + 3);
      _year = EEPROM.read(tempAddr + 4);
      _hour = EEPROM.read(tempAddr + 5);
      _minute = EEPROM.read(tempAddr + 6);
      Serial.print("Recorded: ");
      printDate(_month,_day,_year,_hour,_minute);
    }
    
    //LOWEST HUMIDITY
    else if(tokenBuffer[1] == tLO && tokenBuffer[2] == tEOL){
      temp_min_max = 250;
      tempAddr = 0;
      for(i = 0; i < MAX_ENTRY_ADDR; i += NEXT_ENTRY){
        tempf = EEPROM.read(i + 1);
        if(tempf <= temp_min_max && tempf > 0){
          temp_min_max = tempf;
          tempAddr = i;
        }
      }
      Serial.print("The lowest recorded temperature is: ");
      Serial.print(temp_min_max); Serial.println(" *F");
      _month = EEPROM.read(tempAddr + 2);
      _day = EEPROM.read(tempAddr + 3);
      _year = EEPROM.read(tempAddr + 4);
      _hour = EEPROM.read(tempAddr + 5);
      _minute = EEPROM.read(tempAddr + 6);
      Serial.print("Recorded: ");
      printDate(_month,_day,_year,_hour,_minute);
    }
    else
      Serial.println("Command not recognized");
  }
  
  
  //ERASE TEMPERATURE AND HUMIDITY LOG HISTORY
  else if(tokenBuffer[0] == tER){
    for(i = 0; i < EEPROM.length(); i++){
      EEPROM.put(i, '\0');
    }
    curEepromAddr = 0;
    EEPROM.put(EEPROM_LATEST_ENTRY_ADDR, curEepromAddr);
    Serial.println("EEPROM Memory Erased");
  }
  
  //PRINT TEMPERATURE AND HUMIDITY LOG HISTORY
  else if(tokenBuffer[0] == tPR){
    Serial.println("EEPROM Contents");
    Serial.print("current write address: "); Serial.print(curEepromAddr);
    Serial.print(" (Entry #"); Serial.print(curEepromAddr/7);Serial.println(")");
    count = 0;
    for(i = 0; i < MAX_ENTRY_ADDR; i += NEXT_ENTRY){
      if(EEPROM.read(i + 1) > 0){
        Serial.print("Entry: ");Serial.println(count);
        Serial.print("  Temperature: "); Serial.print(EEPROM.read(i)); Serial.println("°F");
        Serial.print("  Humidity: "); Serial.print(EEPROM.read(i + 1)); Serial.println("%");

        Serial.print("  Recorded: ");
        Serial.print(EEPROM.read(i + 2));Serial.print("/");
        Serial.print(EEPROM.read(i + 3));Serial.print("/");
        Serial.print("20"); Serial.print(EEPROM.read(i + 4));Serial.print(" -- ");
        if(EEPROM.read(i + 5) > 12){
          Serial.print((EEPROM.read(i + 5)) - 12);Serial.print(":");
          Serial.print(EEPROM.read(i + 6));Serial.println("PM");
        }
        else{
          Serial.print(EEPROM.read(i + 5));Serial.print(":");
          Serial.print(EEPROM.read(i + 6));Serial.println("AM");
        }
        
        count++;
      }
    }
  }
  #endif
  //MOTOR COMMANDS
  else if(tokenBuffer[0] == tMO){
    
    //OFF
    if(tokenBuffer[1] == tOF && tokenBuffer[2] == tEOL){
      Serial.println("MOTOR is now OFF");
    }
    
    //VARIABLE SPEED
    else if(tokenBuffer[1] == tON && tokenBuffer[2] > 127 && tokenBuffer[3] == tEOL){
      Serial.print("MOTOR is now ON: ");
      Serial.println(tokenBuffer[i + 2] - 128);
    }
    
    //ON
    else if(tokenBuffer[1] == tON && tokenBuffer[2] == tEOL){
      Serial.println("MOTOR is now ON");
    }
    else
      Serial.println("Command not recognized");
  }
  
  //VERSION
  else if(tokenBuffer[0] == tVE && tokenBuffer[1] == tEOL){
    Serial.print("Arduino Interpreter v");
    Serial.println(VERSION);
    Serial.println("September 3, 2017");
    Serial.println("Brian Geiger");
  }
  
  //ADD
  else if(tokenBuffer[0] == tAD && tokenBuffer[1] > 127 && tokenBuffer[2] > 127 && tokenBuffer[3] == tEOL){
    Serial.print(tokenBuffer[1] - 128);
    Serial.print(" + ");
    Serial.print(tokenBuffer[2] - 128);
    Serial.print(" = ");
    Serial.println((tokenBuffer[1] - 128) + (tokenBuffer[2] - 128));
  }

  else
    Serial.println("Command not recognized");
  
  reset();
}
 
void tokenize(byte first, byte last, char buff[]) {
  //Turns word or number into a token (byte)
 
  //NUMBER
  if(buff[first] >= '0' && buff[first] <= '9') {
    //Throw out if it contains non-numerical characters
    for(i = first; i <= last; i++){
      if(buff[i] < '0' || buff[i] > '9'){
        Serial.print("<");
        for(i = first; i <= last; i++){
          Serial.print(buff[i]);
        }
        Serial.print("> ");
        Serial.print(tERR);
        Serial.print(" (Parse Error: Number Contains Non-Numbers)");
        Serial.println();
        tokenBuffer[currentToken] = tERR;
        currentToken++;
        return;
      }
    }
    //Convert string number to number value
    byte num = 0;
    for(i = first; i <= last; i++){
      num = (10 * num) + (buff[i] - '0');
    }
    //Throw out if larger than 127 or larger than 3 digits
    if(num > 127 || ((last - first + 1) > 3)){
      Serial.print("<");
      for(i = first; i <= last; i++){
        Serial.print(buff[i]);
      }
      Serial.print("> ");
      Serial.print(tERR);
      Serial.print(" (Parse Error: Number Too Large)");
      Serial.println();
      tokenBuffer[currentToken] = tERR;
      currentToken++;
    }
    //Else add 128 and add to token buffer
    else{
      Serial.print("<");
      Serial.print(num);
      Serial.print("> ");
      num += 128;
      Serial.print(num);
      Serial.println();
      tokenBuffer[currentToken] = num;
      currentToken++;
    }
  }

  //WORD
  else{
    //Match to library of words
    byte token = checkToken(first, last, buff);
    Serial.print("<");
    if(token != tERR){
      for(i = first; i <= last; i++){
        Serial.print(buff[i]);
      }
      Serial.print("> ");
      Serial.print(token);
    }
    else{
      for(i = first; i <= last; i++){
        Serial.print(buff[i]);
      }
      Serial.print("> ");
      Serial.print(token);
      Serial.print(" (Parse Error: Unexpected Token)");
    }
    Serial.println();
    tokenBuffer[currentToken] = token;
    currentToken++;
  }
}

byte checkToken(byte first, byte last, char buff[]) {
  //Compare input token to library of declared tokens
  
  i = 0;
  while(TOKS[i] != '\0'){
    if(buff[first] == TOKS[i] && buff[first + 1] == TOKS[i + 1])
      return TOKS[i + 2];
    //case for single letter tokens
    else if(first == last && buff[first] == TOKS[i] && TOKS[i + 1] == '\0'){
      return TOKS[i + 2];
    }
    else
      i += 3;
  }
  return tERR;

};

void reset(){
  //Resets buffers and globals to zero after a parse
  
  memset(tokenBuffer, '\0', sizeof(tokenBuffer));
  memset(inputBuffer, '\0', sizeof(inputBuffer));
  currentToken = 0;
  phase = 0;
  i = 0;
  tokenStart = 0;
  tokenEnd = 0;
  Serial.println("_");
  Serial.println();
}

void printDate(byte mo, byte d, byte y, byte h, byte m){
  //Prints out the date
  
  Serial.print(mo); Serial.print("/");
  Serial.print(d);Serial.print("/");
  Serial.print("20");Serial.print(y);Serial.print(" -- ");
  if(h >= 12){
    Serial.print(h - 12);Serial.print(":");
    if(m >= 10)
      Serial.print(m);
    else
      Serial.print("0");Serial.print(m);
    Serial.println(" PM");
  }
  else{
    Serial.print(h);Serial.print(":");
    if(m >= 10)
      Serial.print(m);
    else
      Serial.print("0");Serial.print(m);
    Serial.println(" AM");
  }
}

byte rotateByteLeft(byte b){
  return(b & (1 << 7)) ? ((b <<= 1) |= 1) : (b <<= 1);
}

void setup() {
  
  Serial.begin(9600);
  Serial.println("Arduino Interpreter - Brian Geiger");
  Serial.print("Version "); Serial.println(VERSION); Serial.println();
  
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(EXT_LED_PIN_CATHODE, OUTPUT);
  pinMode(EXT_LED_PIN_ANODE, OUTPUT);

  #ifdef dhtON
  dht.begin();
  //Set the current write address to the one saved on EEPROM
  EEPROM.get(EEPROM_LATEST_ENTRY_ADDR, curEepromAddr);
  
  //RTC setup
  Clock.begin();
  //MyDateAndTime = Clock.read();
  //_month = MyDateAndTime.Month;
  //_day = MyDateAndTime.Day;
  //_year = MyDateAndTime.Year;
  //_hour = MyDateAndTime.Hour;
  //_minute = MyDateAndTime.Minute;
  //Print date and time
  //printDate(_month,_day,_year,_hour,_minute);
  #endif
  
  //UDP setup
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  ether.staticSetup(myip, gwip);
  
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  
  //register udpSerialPrint() to port 2001
  ether.udpServerListenOnPort(&udpSerialPrint, 2001);
  //register udpSerialPrint() to port 42.
  ether.udpServerListenOnPort(&udpSerialPrint, 42);

  //LED setup
  //strip.begin();
  FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(leds, NUM_LEDS);
  FastLED.show(); // Initialize all pixels to 'off'
  for(i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB(0,120,0);
  }
  FastLED.show();

  //LCD setup
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Box OS v");lcd.print(VERSION);
  lcd.setCursor(0, 1);
  
  reset();
}


void loop() { 
  
  //Serial.println("Interpreter");
  switch(phase) {
    case 0:
      getKeyboardInput();
    break;
    case 1:
      parseTokens();
    break;
    case 2:
      executeCommand();
    break;
  }
  
  //Serial.println("Board LEDs");
  //ledStateMachine();

  //Serial.println("UDP Listener");
  ether.packetLoop(ether.packetReceive());
  
  //Serial.println("Temp State Machine");
  //dhtStateMachine();

  //Rainbow
  //rainbowEffect();

  //LCD State Machine
  partyStateMachine();
  

}
