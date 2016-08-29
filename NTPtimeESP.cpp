/*
   NTP
   This routine gets the unixtime from a NTP server and adjusts it to the time zone and the
   Middle European summer time if requested

  Author: Andreas Spiess V1.0 2016-5-28

  Based on work from John Lassen: http://www.john-lassen.de/index.php/projects/esp-8266-arduino-ide-webconfig

*/

#include <Arduino.h>
#include "NTPtimeESP.h"

#define LEAP_YEAR(Y) ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

const int NTP_PACKET_SIZE = 48;
byte _packetBuffer[ NTP_PACKET_SIZE];
static const uint8_t _monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

float _timeZone=0.0;
String _NTPserver="";

// NTPserver is the name of the NTPserver

NTPtime::NTPtime(String NTPserver) {
  _NTPserver = NTPserver;
}

void NTPtime::printDateTime(strDateTime _dateTime) {

  Serial.print(_dateTime.year);
  Serial.print( "-");
  Serial.print(_dateTime.month);
  Serial.print( "-");
  Serial.print(_dateTime.day);
  Serial.print( "-");
  Serial.print(_dateTime.dayofWeek);
  Serial.print( " ");

  Serial.print(_dateTime.hour);
  Serial.print( "H ");
  Serial.print(_dateTime.minute);
  Serial.print( "M ");
  Serial.print(_dateTime.second);
  Serial.print( "S ");
  Serial.println();
}

// Converts a unix time stamp to a strDateTime structure
strDateTime NTPtime::ConvertUnixTimestamp( unsigned long _tempTimeStamp) {
  strDateTime _tempDateTime;
  uint8_t _year, _month, _monthLength;
  uint32_t _time;
  unsigned long _days;

  _time = (uint32_t)_tempTimeStamp;
  _tempDateTime.second = _time % 60;
  _time /= 60; // now it is minutes
  _tempDateTime.minute = _time % 60;
  _time /= 60; // now it is hours
  _tempDateTime.hour = _time % 24;
  _time /= 24; // now it is _days
  _tempDateTime.dayofWeek = ((_time + 4) % 7) + 1;  // Sunday is day 1

  _year = 0;
  _days = 0;
  while ((unsigned)(_days += (LEAP_YEAR(_year) ? 366 : 365)) <= _time) {
    _year++;
  }
  _tempDateTime.year = _year; // year is offset from 1970

  _days -= LEAP_YEAR(_year) ? 366 : 365;
  _time  -= _days; // now it is days in this year, starting at 0

  _days = 0;
  _month = 0;
  _monthLength = 0;
  for (_month = 0; _month < 12; _month++) {
    if (_month == 1) { // february
      if (LEAP_YEAR(_year)) {
        _monthLength = 29;
      } else {
        _monthLength = 28;
      }
    } else {
      _monthLength = _monthDays[_month];
    }

    if (_time >= _monthLength) {
      _time -= _monthLength;
    } else {
      break;
    }
  }
  _tempDateTime.month = _month + 1;  // jan is month 1
  _tempDateTime.day = _time + 1;     // day of month
  _tempDateTime.year += 1970;

  return _tempDateTime;
}


//
// Summertime calculates the daylight saving time for middle Europe. Input: Unixtime in UTC
//
boolean NTPtime::summerTime(unsigned long _timeStamp ) {

  strDateTime  _tempDateTime;
  _tempDateTime = ConvertUnixTimestamp(_timeStamp);
  // printTime("Innerhalb ", _tempDateTime);

  if (_tempDateTime.month < 3 || _tempDateTime.month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (_tempDateTime.month > 3 && _tempDateTime.month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (_tempDateTime.month == 3 && (_tempDateTime.hour + 24 * _tempDateTime.day) >= (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 4) % 7)) || _tempDateTime.month == 10 && (_tempDateTime.hour + 24 * _tempDateTime.day) < (3 +  24 * (31 - (5 * _tempDateTime.year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

boolean NTPtime::daylightSavingTime(unsigned long _timeStamp) {

  strDateTime  _tempDateTime;
  _tempDateTime = ConvertUnixTimestamp(_timeStamp);

// here the US code
    return false;
}


unsigned long NTPtime::adjustTimeZone(unsigned long _timeStamp, float _timeZone, byte _DayLightSaving) {
  strDateTime _tempDateTime;
  _timeStamp += (unsigned long)(_timeZone *  3600.0); // adjust timezone
  if (_DayLightSaving ==1 && summerTime(_timeStamp)) _timeStamp += 3600; // European Summer time
  if (_DayLightSaving ==2 && daylightSavingTime(_timeStamp)) _timeStamp += 3600; // US daylight time
  return _timeStamp;
}

// time zone is the difference to UTC in hours
// if _isDayLightSaving is true, time will be adjusted accordingly

strDateTime NTPtime::getNTPtime(float _timeZone, boolean _DayLightSaving)
{
  int cb;
  strDateTime _dateTime;
  unsigned long _unixTime = 0;
  _dateTime.valid = false;
  unsigned long _currentTimeStamp;

  if (WiFi.status() == WL_CONNECTED)
  {
    #ifdef DEBUG_ON
       Serial.println("Waiting for NTP packet");
    #endif
    UDPNTPClient.begin(1337);  // Port for NTP receive
    
    #ifdef DEBUG_ON
      IPAddress _timeServerIP;
 -    WiFi.hostByName(_NTPserver.c_str(), _timeServerIP);
      Serial.println(_timeServerIP);
    #endif
 
    while (!_dateTime.valid) {

    #ifdef DEBUG_ON
      Serial.println("sending NTP packet...");
    #endif
      memset(_packetBuffer, 0, NTP_PACKET_SIZE);
      _packetBuffer[0] = 0b11100011;   // LI, Version, Mode
      _packetBuffer[1] = 0;     // Stratum, or type of clock
      _packetBuffer[2] = 6;     // Polling Interval
      _packetBuffer[3] = 0xEC;  // Peer Clock Precision
      _packetBuffer[12]  = 49;
      _packetBuffer[13]  = 0x4E;
      _packetBuffer[14]  = 49;
      _packetBuffer[15]  = 52;
      UDPNTPClient.beginPacket(_NTPserver.c_str(), 123);
      UDPNTPClient.write(_packetBuffer, NTP_PACKET_SIZE);
      UDPNTPClient.endPacket();

      unsigned long entry=millis();
      do {
         cb = UDPNTPClient.parsePacket();
      } while (cb == 0 && millis()-entry<5000);
      
      if (cb == 0) {
         #ifdef DEBUG_ON
            Serial.print(".");
         #endif
      }
      else {
        #ifdef DEBUG_ON
           Serial.println();
           Serial.print("NTP packet received, length=");
           Serial.println(cb);
        #endif
        UDPNTPClient.read(_packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        unsigned long highWord = word(_packetBuffer[40], _packetBuffer[41]);
        unsigned long lowWord = word(_packetBuffer[42], _packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        _unixTime = secsSince1900 - seventyYears;
        if (_unixTime > 0) {
          _currentTimeStamp = adjustTimeZone(_unixTime, _timeZone, _DayLightSaving);
          _dateTime = ConvertUnixTimestamp(_currentTimeStamp);
          _dateTime.valid = true;
        } else _dateTime.valid = false;
      }
    }
  } else {
    #ifdef DEBUG_ON
       Serial.println("NTP: Internet not yet connected");
    #endif
    delay(500);
  }
  yield();
  return _dateTime;
}
