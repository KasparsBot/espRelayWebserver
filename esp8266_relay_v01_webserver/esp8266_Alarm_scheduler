/*
 * TimeAlarmExample.pde
 *
 * This example calls alarm functions at 8:30 am and at 5:45 pm (17:45)
 * and simulates turning lights on at night and off in the morning
 * A weekly timer is set for Saturdays at 8:30:30
 *
 * A timer is called every 15 seconds
 * Another timer is called once only after 10 seconds
 *
 * At startup the time is set to Jan 1 2011  8:29 am
 */
#include <ESP8266WiFi.h>
#include <Time.h>
#include <TimeAlarms.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient (ntpUDP);
WiFiServer server(80);

const char* ssid = "wifi_ssid";//type your ssid
const char* password = "wifi_passwd";//type your password

bool ledStatus = false;
int ledPin = 2; // GPIO2 of ESP8266

void setup()
{
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Alarm.delay(500);
    Serial.print(".");
    //TimerRepeats();
    yield();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Sync time with NTP server
  timeClient.begin();
  
  /*
  timeClient.update();
  setTime(8,29,0,1,1,11); // set time to Saturday 8:29:00am Jan 1 2011
  setTime(timeClient.getHours(),timeClient.getMinutes(),0,1,1,21); // v1.10 - set time to XX:YY:00am Jan 1 2021
  */
  Alarm_SyncNTPTime();

  // Start the webServer
  server.begin();
  Serial.println("Server started");

  // SYSTEM Alarms
  // - Daily NTP sync to ensure that system time is in sync      
    Alarm.alarmRepeat(0,00,0, Alarm_SyncNTPTime);  // 00:00 every day


  // ALARM EXAMPLES
  //  ---
  //  Alarm.timerOnce([interval_in_seconds], [functionName]); // triggers alarm function [functionName] every [interval_in_seconds] seconds.
  //  Alarm.timerOnce(10, OnceOnly); // triggers alarm function 'OnceOnly' every 10 seconds.
  //  ---
  //  Alarm.alarmRepeat([Hours],[Min],[Sec], [functionName]);  // triggers alarm function [functionName] at [Hours]:[Min]:[Sec] every day.
  //  Alarm.alarmRepeat(8,30,0, MorningAlarm);  // 08:30 every day
  //  ---
  //  Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday
  //  
  //  ==============================================================================

  // CREATE CUSTOM ALARMS 
  Alarm.timerRepeat(5, Alarm_BlinkLED);            // timer for every 5 seconds  
  Alarm.timerRepeat(15, Repeats);            // timer for every 15 seconds  
  Alarm.timerOnce(10, OnceOnly);             // called once after 10 seconds
}

void  loop(){
  digitalClockDisplay();
  //Serial.print(".");

  // Check if a client has connected

  Alarm.delay(1000); // wait one second between clock display
} // loop()

// functions to be called when an alarm triggers:
void Alarm_SyncNTPTime(){
  timeClient.update();
  setTime(timeClient.getHours(),timeClient.getMinutes(),0,1,1,21); // v1.10 - set time to XX:YY:00am Jan 1 2021
}

void Alarm_BlinkLED(){
  ledStatus = !ledStatus;
  digitalWrite(LED_BUILTIN, ledStatus);
  Serial.print(timeClient.getFormattedTime());
  Serial.println(": Alarm_BlinkLED");
  yield();        
}


void MorningAlarm(){
  Serial.println("Alarm: - turn lights off");    
}

void EveningAlarm(){
  Serial.println("Alarm: - turn lights on");           
}

void WeeklyAlarm(){
  Serial.println("Alarm: - its Monday Morning");      
}

void ExplicitAlarm(){
  Serial.println("Alarm: - this triggers only at the given date and time");       
}

void Repeats(){
  Serial.print(timeClient.getFormattedTime());
  Serial.println(": 15 second timer");         
}

void OnceOnly(){
  Serial.print(timeClient.getFormattedTime());
  Serial.println("This timer only triggers once");  
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println(); 
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
