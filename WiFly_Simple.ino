// 6 April 2012
// Anthony Roberts
// Simple program to test communication with RN-XV WiFly Module

#include <SoftwareSerial.h>
#include <Time.h>

/*
WiFly Pin  1 (POWER) to 3.3v on Arduino (or other 3.3v Power Supply)
WiFly Pin  2 (TX)    to Arduino Pin 2 (SoftSerial RX)
WiFly Pin  3 (RX)    to Arduino Pin 3 (SoftSerial TX)
WiFly Pin 10 (GND)   to Ground

This program won't make any changes to the WiFly - it just connects

After uploading, open the Serial Monitor Window and make sure it's set to No line ending and 9600 baud
Enter $$$ and click Send (you should get a CMD displayed)
Switch from No line ending to Carriage Return
Enter get mac and press Return (you should get Mac Addr=00:06:66:71:e8:50 followed by <2.32>)

Useful (or maybe just interesting) Commands
scan                    View a list of visible WiFi networks
show time               Show the time
time                    Synchronise the time with the NTP server (assuming it's configured)
get ip                  Show IP details such as leased IP address
lites                   Flash the LEDs on the WiFly (use same command again to turn off)
ping 8.8.8.8 5          Ping 8.8.8.8 5 times
lookup www.apple.com    Use DNS to resolve www.apple.com
show net
show rssi
show stats
*/

#define ARDUINO_RX 2
#define ARDUINO_TX 3
#define DIGITAL 0
#define ANALOG 1

#define ACCESS_ALLOW 5705    // Are you old enough to remember this song?

long RTC_Time = 0;
byte redMode = LOW;
byte greenMode = LOW;

// LED's are numbered 1 to 9, but array elements start at 0. To make things simple, I don't use element 0. 
int ledPins[10] = {0,4,5,6,7,8,9,10,11,12};   // Put a Zero for unused pins (or any that are assigned to another function)
int ledState[10] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int ledType[10] = {0, DIGITAL, ANALOG, ANALOG, DIGITAL, DIGITAL, ANALOG, ANALOG, ANALOG, DIGITAL};

SoftwareSerial WiFly(ARDUINO_RX, ARDUINO_TX);

void setup()
{
  for (int i = 1; i <= 9; i++) {
    if (ledPins[i] > 0) {
      pinMode(ledPins[i], OUTPUT);
      digitalWrite(ledPins[i], ledState[i]);
    }
  }
    
// Make sure all the LED's are working (only happens at power up)
  for (int i = 1; i <= 9; i++) {
    if (ledPins[i] > 0) {
      digitalWrite(ledPins[i], HIGH);
    }
  }
  delay(1000);
  for (int i = 1; i <= 9; i++) {
    if (ledPins[i] > 0) {
      digitalWrite(ledPins[i], LOW);
    }
  }
  delay(1000);
  for (int i = 1; i <= 9; i++) {
    if (ledPins[i] > 0) {
      digitalWrite(ledPins[i], ledState[i]);
    }
  }  

  Serial.begin(9600);
  getTemp();
  Serial.println("Connecting to the WiFly");
  
  WiFly.begin(9600);
  findVersion();
  setSyncProvider(getRTC);
  showTime();
  
  Serial.println("OK, now it's your turn. Enter $$$ to begin Command Mode");
}

void showTime() 
{
  char* moty[]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char* dotw[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  
  Serial.print("Time is ");
  Serial.print(hour());
  Serial.print(':');
  Serial.print(minute());
  Serial.print(':');
  Serial.println(second());

  Serial.print("Date is ");
  Serial.print(dotw[weekday()-1]);
  Serial.print(", ");
  Serial.print(day());
  Serial.print(' ');
  Serial.print(moty[month()]);
  Serial.print(' ');
  Serial.println(year());
  
  WiFly.print("TIME:");
  printDigits(hour());
  WiFly.print(":");
  printDigits(minute());
  WiFly.print(":");
  printDigits(second());
  WiFly.println("");
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    WiFly.print('0');
  WiFly.print(digits);
}

void loop()
{
  char instruc, param1, param2, param3, param4;
  int whichPin, brightness, accessCode;
  
  if (WiFly.available()) {
    if (WiFly.peek() == '?') {
      WiFly.print("STATE:");
      for (int i = 1; i <= 9; i++) {
        if (ledState[i] == LOW) WiFly.print("0"); else WiFly.print("1");
      }
      WiFly.println("");
      Serial.write(WiFly.read());
      return;
    }
    
    if (WiFly.peek() == '!') {  // Sending an instruction to do something. Will be followed by two character command
      delay(100);     // Allow time for the rest of the instruction to arrive (Arduino's can be too fast sometimes!)
      Serial.write(WiFly.read());
      instruc = WiFly.read();    // eg. H for Set High on LED (with LED 0-9 as the parameter)
      param1 = WiFly.read();      // May not be required - just send anything if not needed
      param2 = WiFly.read();
      param3 = WiFly.read();
      param4 = WiFly.read();
      
      switch (instruc) {
        case 'H':
          whichPin = param1 - 48;        // 0 is Ascii 48, so subtract 48 to convert to an actual number
          if (ledPins[whichPin] > 0) {
            digitalWrite(ledPins[whichPin], HIGH);
            ledState[whichPin] = HIGH;
          }
          break;
        
        case 'L':
          whichPin = param1 - 48;        // 0 is Ascii 48, so subtract 48 to convert to an actual number
          if (ledPins[whichPin] > 0) {
            digitalWrite(ledPins[whichPin], LOW);
            ledState[whichPin] = LOW;
          }
          break;
        
        case 'A':
          whichPin = param1 - 48;        // 0 is Ascii 48, so subtract 48 to convert to an actual number
          brightness = ((param2 - 48) * 100) + ((param3 - 48) * 10) + (param4 - 48);
          if (ledPins[whichPin] > 0 && ledType[whichPin] == ANALOG) {
            analogWrite(ledPins[whichPin], brightness);
            if (brightness == 0) ledState[whichPin] = LOW; else ledState[whichPin] = HIGH;
          }
          break;
        
        case 'S':
          accessCode = ((param1 - 48) * 1000) + ((param2 - 48) * 100) + ((param3 - 48) * 10) + (param4 - 48);
          if (accessCode == ACCESS_ALLOW) {
            WiFly.println("ACCESS:ALLOW");
            // Would really need to do something here now that they've got the correct access code
          } else {
            WiFly.println("ACCESS:DENY");
          }
          break;

        case 'C':
          getTemp();
          break;
        
        case 'T':
          getRTC();
          showTime();
          break;
        
        default:
          break;   // Do nothing if we don't recognise the instruction
      }
      return;
    }
              
    Serial.write(WiFly.read());
  }
  
  if (Serial.available()) {
    if (Serial.peek() == 'T') {
      showTime();
    }
    if (Serial.peek() == 'U') {
      getRTC();
    }
    WiFly.write(Serial.read());
  }
}

void flash(int howMany)
{
  for (int i = 0; i < howMany; i++) {
    digitalWrite(4, HIGH);
    delay(250);
    digitalWrite(4, LOW);
    delay(100);
  }
}


void findVersion() 
{
  char WiFlyBuffer[100];
  char c;
  byte i = 0;
  
  Serial.println("Getting the Version of the WiFly");
  WiFly.write("$$$");
  delay(500);
  WiFly.flush();
  WiFly.write("ver\r");
  delay(100);
  while (WiFly.available()) {
    c = WiFly.read();
    WiFlyBuffer[i++] = c;
    if (c == '\r') break;
    if (i > 99) break;
  }
  WiFlyBuffer[i] = 0;
  WiFly.flush();
  WiFly.write("exit\r");
  delay(100);
  WiFly.flush();
  Serial.print("The Firmware Version is ");
  Serial.println(WiFlyBuffer);
}


time_t getRTC() {
  char WiFlyBuffer[100];
  char RTC[20];
  char c;
  byte i = 0;
  
  Serial.println("Getting the Time");
  WiFly.write("$$$");
  delay(500);
  WiFly.flush();
  WiFly.write("show time t\r");
  delay(100);
  while (WiFly.available()) {
    c = WiFly.read();
    WiFlyBuffer[i++] = c;
    if (c == '>') break;
    if (i > 99) break;
  }
  WiFlyBuffer[i] = 0;
  WiFly.flush();
  WiFly.write("exit\r");
  delay(100);
  WiFly.flush();

  byte startRTC = 0;
  for (i = 0; i < 95; i++) {
    if (WiFlyBuffer[i] == 0) break;
    if (startRTC > 0 && WiFlyBuffer[i] == 13) {
      RTC[i] = 0;
      break;
    }
    if (WiFlyBuffer[i] == 'R' && WiFlyBuffer[i+1] == 'T' && WiFlyBuffer[i+2] == 'C') {
      startRTC = i + 4;
      i = i + 4;
    }
    if (startRTC > 0) RTC[i - startRTC] = WiFlyBuffer[i];
  }

  RTC_Time = atol(RTC);
  RTC_Time = RTC_Time + (1 * 60 * 60);
  setTime(RTC_Time);

  return RTC_Time;
}

void getTemp() 
{
  float ap0 = analogRead(A0) * 0.004882814;
  float tdc = ((ap0 - 0.5) * 100) - 0.80;
  WiFly.print("TEMP:");
  WiFly.println(tdc);
}

