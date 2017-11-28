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

#define SEC_TO_MS             1000
#define RECV_TIMEOUT_DEFATUL  1       // 1 second
#define SEND_INTRVL_DEFAULT   1       // 1 second
#define MAX_SEND_INTERVAL     60      // 60 seconds
#define MAC_RECV_TIMEOUT      60      // 60 seconds

const int NTP_PACKET_SIZE = 48;
byte _packetBuffer[ NTP_PACKET_SIZE];
static const uint8_t _monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

float _timeZone=0.0;
String _NTPserver="";

// NTPserver is the name of the NTPserver

bool NTPtime::setSendInterval(unsigned long _sendInterval_) {
	bool retVal = false;
	if(_sendInterval_ <= MAX_SEND_INTERVAL) {
		_sendInterval = _sendInterval_ * SEC_TO_MS;
		retVal = true;
	}

	return retVal;
}

bool NTPtime::setRecvTimeout(unsigned long _recvTimeout_) {
	bool retVal = false;
	if(_recvTimeout_ <= MAC_RECV_TIMEOUT) {
		_recvTimeout = _recvTimeout_ * SEC_TO_MS;
		retVal = true;
	}

	return retVal;  
}

NTPtime::NTPtime(String NTPserver) {
	_NTPserver = NTPserver;
	_sendPhase = true;
	_sentTime  = 0;
	_sendInterval = SEND_INTRVL_DEFAULT * SEC_TO_MS;
	_recvTimeout = RECV_TIMEOUT_DEFATUL * SEC_TO_MS;
}

void NTPtime::printDateTime(strDateTime _dateTime) {
	if (_dateTime.valid) {
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
	} else {
#ifdef DEBUG_ON
		Serial.println("Invalid time !!!");
		Serial.println("");
#endif    
	}
}

// Converts a unix time stamp to a strDateTime structure
strDateTime NTPtime::ConvertUnixTimestamp( unsigned long _tempTimeStamp) {
	strDateTime _tempDateTime;
	uint8_t _year, _month, _monthLength;
	uint32_t _time;
	unsigned long _days;
	
	_tempDateTime.epochTime = _tempTimeStamp;

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
	//return false;
	// see http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date
	// since 2007 DST begins on second Sunday of March and ends on first Sunday of November. 
	// Time change occurs at 2AM locally
	if (_tempDateTime.month < 3 || _tempDateTime.month > 11) return false;  //January, february, and december are out.
	if (_tempDateTime.month > 3 && _tempDateTime.month < 11) return true;   //April to October are in
	int previousSunday = _tempDateTime.day - (_tempDateTime.dayofWeek - 1);  // dow Sunday input was 1,
	// need it to be Sunday = 0. If 1st of month = Sunday, previousSunday=1-0=1
	//int previousSunday = day - (dow-1);
	// -------------------- March ---------------------------------------
	//In march, we are DST if our previous Sunday was = to or after the 8th.
	if (_tempDateTime.month == 3 ) {  // in march, if previous Sunday is after the 8th, is DST
		// unless Sunday and hour < 2am
		if ( previousSunday >= 8 ) { // Sunday = 1
			// return true if day > 14 or (dow == 1 and hour >= 2)
			return ((_tempDateTime.day > 14) || 
			((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour >= 2) || _tempDateTime.dayofWeek > 1));
		} // end if ( previousSunday >= 8 && _dateTime.dayofWeek > 0 )
		else
		{
			// previousSunday has to be < 8 to get here
			//return (previousSunday < 8 && (_tempDateTime.dayofWeek - 1) = 0 && _tempDateTime.hour >= 2)
			return false;
		} // end else
	} // end if (_tempDateTime.month == 3 )

	// ------------------------------- November -------------------------------

	// gets here only if month = November
	//In november we must be before the first Sunday to be dst.
	//That means the previous Sunday must be before the 2nd.
	if (previousSunday < 1)
	{
		// is not true for Sunday after 2am or any day after 1st Sunday any time
		return ((_tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2) || (_tempDateTime.dayofWeek > 1));
		//return true;
	} // end if (previousSunday < 1)
	else
	{
		// return false unless after first wk and dow = Sunday and hour < 2
		return (_tempDateTime.day <8 && _tempDateTime.dayofWeek == 1 && _tempDateTime.hour < 2);
	}  // end else
} // end boolean NTPtime::daylightSavingTime(unsigned long _timeStamp)


unsigned long NTPtime::adjustTimeZone(unsigned long _timeStamp, float _timeZone, int _DayLightSaving) {
	strDateTime _tempDateTime;
	_timeStamp += (unsigned long)(_timeZone *  3600.0); // adjust timezone
	if (_DayLightSaving ==1 && summerTime(_timeStamp)) _timeStamp += 3600; // European Summer time
	if (_DayLightSaving ==2 && daylightSavingTime(_timeStamp)) _timeStamp += 3600; // US daylight time
	return _timeStamp;
}

// time zone is the difference to UTC in hours
// if _isDayLightSaving is true, time will be adjusted accordingly
// Use returned time only after checking "ret.valid" flag

strDateTime NTPtime::getNTPtime(float _timeZone, int _DayLightSaving) {
	int cb;
	strDateTime _dateTime;
	unsigned long _unixTime = 0;
	_dateTime.valid = false;
	unsigned long _currentTimeStamp;

	if (_sendPhase) {
		if (_sentTime && ((millis() - _sentTime) < _sendInterval)) {
			return _dateTime;
		}

		_sendPhase = false;
		UDPNTPClient.begin(1337); // Port for NTP receive

#ifdef DEBUG_ON
		IPAddress _timeServerIP;
		WiFi.hostByName(_NTPserver.c_str(), _timeServerIP);
		Serial.println();
		Serial.println(_timeServerIP);
		Serial.println("Sending NTP packet");
#endif

		memset(_packetBuffer, 0, NTP_PACKET_SIZE);
		_packetBuffer[0] = 0b11100011; // LI, Version, Mode
		_packetBuffer[1] = 0;          // Stratum, or type of clock
		_packetBuffer[2] = 6;          // Polling Interval
		_packetBuffer[3] = 0xEC;       // Peer Clock Precision
		_packetBuffer[12] = 49;
		_packetBuffer[13] = 0x4E;
		_packetBuffer[14] = 49;
		_packetBuffer[15] = 52;
		UDPNTPClient.beginPacket(_NTPserver.c_str(), 123);
		UDPNTPClient.write(_packetBuffer, NTP_PACKET_SIZE);
		UDPNTPClient.endPacket();

		_sentTime = millis();
	} else {
		cb = UDPNTPClient.parsePacket();
		if (cb == 0) {
			if ((millis() - _sentTime) > _recvTimeout) {
				_sendPhase = true;
				_sentTime = 0;
			}
		} else {
#ifdef DEBUG_ON
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
			} else
			_dateTime.valid = false;

			_sendPhase = true;
		}
	}

	return _dateTime;
}
