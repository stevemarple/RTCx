/*

 */

#include <Wire.h>
#include <RTCx.h>

void printTm(Stream &str, struct RTCx::tm *tm)
{
  Serial.print(tm->tm_year + 1900);
  Serial.print('-');
  Serial.print(tm->tm_mon + 1);
  Serial.print('-');
  Serial.print(tm->tm_mday);
  Serial.print('T');
  Serial.print(tm->tm_hour);
  Serial.print(':');
  Serial.print(tm->tm_min);
  Serial.print(':');
  Serial.print(tm->tm_sec);
  Serial.print(" yday=");
  Serial.print(tm->tm_yday);
  Serial.print(" wday=");
  Serial.println(tm->tm_wday);
}

void setup(void)
{
  Serial.begin(9600);
  Wire.begin();
  uint8_t addressList[] = {RTCx_MCP7941x_ADDRESS,
			   RTCx_DS1307_ADDRESS};

  // Autoprobe to find a real-time clock.
  if (rtc.autoprobe(addressList, sizeof(addressList))) {
    // Found something, hopefully a clock.
    Serial.print("Autoprobe found ");
    switch (rtc.getDevice()) {
      case RTCx::DS1307:
	Serial.print("DS1307");
	break;
    case RTCx::MCP7941x:
      Serial.print("MCP7941x");
      break;
    default:
      // Ooops. Must update this example!
      Serial.print("unknown device");
      break;
    }
    Serial.print(" at 0x");
    Serial.println(rtc.getAddress(), HEX);
  }
  else {
    // Nothing found at any of the addresses listed.
    Serial.println("No RTCx found");
    return;
  }

  // Enable the battery backup. This happens by default on the DS1307
  // but needs to be enabled on the MCP7941x.
  rtc.enableBatteryBackup();

  // rtc.clearVBAT();
  
  // Ensure the oscillator is running.
  rtc.startClock();

  if (rtc.getDevice() == RTCx::MCP7941x) {
    Serial.print("Calibration: ");
    Serial.println(rtc.getCalibration(), DEC);
    // rtc.setCalibration(-127);
  }

  rtc.setSQW(RTCx::freq4096Hz);
}


const uint8_t bufLen = 30;
char buffer[bufLen + 1] = {'\0'};
uint8_t bufPos = 0;
unsigned long last = 0;
void loop(void)
{
  struct RTCx::tm tm;
  if (millis() - last > 2000) {
    last = millis();
    rtc.readClock(tm);
    
    printTm(Serial, &tm);
    RTCx::time_t t = RTCx::mktime(&tm);
    printTm(Serial, &tm);
    Serial.print("unixtime = ");
    Serial.println(t);
    /*
    Serial.println("...............");
    struct RTCx::tm tm2;
    RTCx::time_t t2 = 1347750631;
    Serial.print("unixtime = ");
    Serial.println(t2);
    RTCx::gmtime_r(&t2, &tm2);
    printTm(Serial, &tm2);
    RTCx::time_t t2a = RTCx::mktime(&tm2);
    printTm(Serial, &tm2);
    Serial.print("unixtime = ");
    Serial.println(t2a);
    Serial.println("...............");
    */
    /*
    rtc.readClock(tm, RTCx::ALARM0);
    printTm(Serial, &tm);
    rtc.readClock(tm, RTCx::ALARM1);
    printTm(Serial, &tm);
    */
    
    /*
      rtc.readClock(tm, RTCx::TIME_POWER_FAILED);
      printTm(Serial, &tm);
      rtc.readClock(tm, RTCx::TIME_POWER_RESTORED);
      printTm(Serial, &tm);
    */
  
    Serial.println("-----");
  }

  while (Serial.available()) {
    char c = Serial.read();
    if ((c == '\r' || c == '\n')) {
      if (bufPos <= bufLen && buffer[0] == 'C') {
	// Check time error
	buffer[bufPos] = '\0';
	RTCx::time_t pcTime = atol(&(buffer[1]));
	rtc.readClock(&tm);
	RTCx::time_t mcuTime = RTCx::mktime(&tm);
	Serial.print("MCU clock error: ");
	Serial.print(mcuTime - pcTime);
	Serial.println(" s");
	Serial.println("~~~~~");
      }
      if (bufPos <= bufLen && buffer[0] == 'T') {
	// Set time
	buffer[bufPos] = '\0';
	RTCx::time_t t = atol(&(buffer[1])); 
	RTCx::gmtime_r(&t, &tm);
	rtc.setClock(&tm);
	Serial.println("Clock set");
	Serial.println(&(buffer[0]));
	printTm(Serial, &tm);
	Serial.println("~~~~~");
      }

      bufPos = 0;
    }
    else if (bufPos < bufLen)
      // Store character
      buffer[bufPos++] = c; 
  }
}
