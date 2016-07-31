/*

   NTPtime for ESP8266
   This routine gets the unixtime from a NTP server and adjusts it to the time zone and the
   Middle European summer time if requested

  Author: Andreas Spiess V1.0 2016-6-28

  Based on work from John Lassen: http://www.john-lassen.de/index.php/projects/esp-8266-arduino-ide-webconfig

*/
#ifndef NTPtime_h
#define NTPtime_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>


#define DEBUG_ON


struct strDateTime
{
  byte hour;
  byte minute;
  byte second;
  int year;
  byte month;
  byte day;
  byte dayofWeek;
  boolean valid;
};

class NTPtime {
  public:
    NTPtime(String NTPtime);
    strDateTime getNTPtime(float _timeZone, boolean _DayLightSaving);
    void printDateTime(strDateTime _dateTime);

  private:
    strDateTime ConvertUnixTimestamp( unsigned long _tempTimeStamp);
    boolean summerTime(unsigned long _timeStamp );
    boolean daylightSavingTime(unsigned long _timeStamp);
    unsigned long adjustTimeZone(unsigned long _timeStamp, float _timeZone, byte _DayLightSavingSaving);
    WiFiUDP UDPNTPClient;
};
#endif
