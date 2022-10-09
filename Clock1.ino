#include <Wire.h>
#include "RTClib.h"
#include <DeepSleepScheduler.h>
#include "SevSeg.h"
#include <CommandParser.h>

//#define DEBUG

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

SevSeg sevseg; //Initiate a seven segment controller object

char display_str[6];
const char* CL_FMT_STR_DOT = "%02d.%02d\0";
const char* CL_FMT_STR_NO_DOT = "%02d%02d\0";
const char* CL_FMT_STR = CL_FMT_STR_DOT;

typedef CommandParser<5, 1, 10, 16, 32> MyCommandParser;
MyCommandParser parser;

void rtc_setup () {
 while (!Serial); // for Leonardo/Micro/Zero
 Serial.begin(57600);
 if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
 }
 if (! rtc.isrunning()) {
   Serial.println("RTC is NOT running!");
 }
 // following line sets the RTC to the date & time this sketch was compiled
 // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
 // This line sets the RTC with an explicit date & time, for example to set
 // Oct 21, 2014 at 3am you would call:
 //rtc.adjust(DateTime(2022, 10, 1, 18, 55, 0));
}

void sevseg_setup() {
    byte numDigits = 4;  
    byte digitPins[] = {9, 10, 11, 12};
    byte segmentPins[] = {2, 3, 4, 5, 6, 7, 8, 13};
    bool resistorsOnSegments = 0; 
    // variable above indicates that 4 resistors were placed on the digit pins.
    // set variable to 1 if you want to use 8 resistors on the segment pins.
    sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins, resistorsOnSegments);
    sevseg.setBrightness(90);
}

void scheduler_setup() {
  scheduler.schedule(toggleDot);
  scheduler.schedule(render);
  scheduler.schedule(serialListen);
  #ifdef DEBUG
  scheduler.schedule(printDebug);
  #endif
}

void cmd_parser_setup() {
  Serial.begin(57600);
  while (!Serial);

  parser.registerCommand("time", "", &cmd_test);
  parser.registerCommand("date", "", &cmd_test);
  parser.registerCommand("set", "s", &cmd_test);
  //parser.registerCommand("TEST", "sdiu", &cmd_test);
  //Serial.println("registered command: TEST <string> <double> <int64> <uint64>");
  //Serial.println("example: TEST \"\\x41bc\\ndef\" -1.234e5 -123 123");
}

void cmd_test(MyCommandParser::Argument *args, char *response) {
  Serial.print("string: "); Serial.println(args[0].asString);
  //Serial.print("double: "); Serial.println(args[1].asDouble);
  //Serial.print("int64: "); Serial.println(int32_t(args[2].asInt64));
  //Serial.print("uint64: "); Serial.println(uint32_t(args[3].asUInt64));
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void serialListen() {
  if (Serial.available()) {
    char line[32];
    size_t lineLength = Serial.readBytesUntil('\n', line, 32);
    line[lineLength] = '\0';

    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.println(response);
  }

  scheduler.scheduleAt(serialListen, scheduler.getScheduleTimeOfCurrentTask() + 10);
} 

// the setup routine runs once when you press reset:
void setup() {   
  rtc_setup();             
  sevseg_setup();
  scheduler_setup();
  cmd_parser_setup();
}

void printDebug() {
  static char debugBuf[50];
  sprintf(debugBuf, "DEBUG: %s:%s", __DATE__, __TIME__);
  Serial.println(debugBuf);
  scheduler.scheduleAt(printDebug, scheduler.getScheduleTimeOfCurrentTask() + 1000);
}

void toggleDot() {
  if (CL_FMT_STR == CL_FMT_STR_DOT)
    CL_FMT_STR = CL_FMT_STR_NO_DOT;
  else
    CL_FMT_STR = CL_FMT_STR_DOT;
  scheduler.scheduleAt(toggleDot, scheduler.getScheduleTimeOfCurrentTask() + 500);
}

void render() {
  sprintf(display_str, CL_FMT_STR, rtc.now().hour(), rtc.now().minute());
  sevseg.setChars(display_str);
  sevseg.refreshDisplay(); // Must run repeatedly
  scheduler.scheduleAt(render, scheduler.getScheduleTimeOfCurrentTask() + 1);
}

// the loop routine runs over and over again forever:
void loop() {
  scheduler.execute();
}
