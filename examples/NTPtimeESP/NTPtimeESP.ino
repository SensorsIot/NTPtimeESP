/*
   This sketch shows an example of sending a reading to data.sparkfun.com once per day.

   It uses the Sparkfun testing stream so the only customizing required is the WiFi SSID and password.

   The Harringay Maker Space
   License: Apache License v2
*/
#include <NTPtimeESP.h>


NTPtime NTPch("ch.pool.ntp.org");

/*
 * The structure contains following fields:
 * struct strDateTime
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
 */
strDateTime dateTime;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booted");
  Serial.println("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");
}

void loop() {

  // first parameter: Time zone in floating point (for India); second parameter: 1 for European summer time; 2 for US daylight saving time (not implemented yet)
  dateTime = NTPch.getNTPtime(1.0, 1);
  NTPch.printDateTime(dateTime);

  byte actualHour = dateTime.hour;
  byte actualMinute = dateTime.minute;
  byte actualsecond = dateTime.second;
  int actualyear = dateTime.year;
  byte actualMonth = dateTime.month;
  byte actualday =dateTime.day;
  byte actualdayofWeek = dateTime.dayofWeek;
  delay(60000);
}
