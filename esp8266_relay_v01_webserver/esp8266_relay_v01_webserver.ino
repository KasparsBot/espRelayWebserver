#include <ESP8266WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

 
const char* ssid = "wifi_ssid";//type your ssid
const char* password = "wifi_passwd";//type your password
 
int ledPin = 2; // GPIO2 of ESP8266
WiFiServer server(80);

char scheduleHours[24];
 
void setup() {

  // Setting up time schedule
  for (int i=0;i<12;i++) {
    scheduleHours[8+i] = 'Y';
   }
  
  Serial.begin(115200);
  delay(10);
 
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
   
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Sync time with NTP server
  timeClient.begin();
   
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
    
}
 
void loop() {
  // Get and output NTP time
  timeClient.update();
  Serial.print(timeClient.getFormattedTime());
  Serial.print(" | schedule=");
  Serial.print(scheduleHours[timeClient.getHours()]);
    
  if(scheduleHours[timeClient.getHours()] == 'Y' ){
    Serial.print(" | ON");
  } else {
    ESP.deepSleep(900e6, WAKE_RFCAL);
  }
  Serial.println("");
  delay(1000);
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
   
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
   
  // Match the request
 
  int value = LOW;
  if (request.indexOf("/LED=ON") != -1) {
    digitalWrite(ledPin, HIGH);
    value = HIGH;
  } 
  if (request.indexOf("/LED=OFF") != -1){
    digitalWrite(ledPin, LOW);
    value = LOW;
  }
 
// Set ledPin according to the request
//digitalWrite(ledPin, value);
   
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
   
  client.print("Led pin is now: ");
   
  if(value == HIGH) {
    client.println("On");
    client.println("<br><br>");
    client.println("<button type=\"button\" onclick=\"window.location.href='/LED=OFF';\">Turn OFF!</button>");
  } else {
    client.println("Off");
    client.println("<br><br>");
    client.println("<button type=\"button\" onclick=\"window.location.href='/LED=ON';\">Turn ON!</button>");
  }
  client.println("<br><br>");
  client.println("Click <a href=\"/LED=ON\">here</a> turn the LED on pin 2 ON<br>");
  client.println("Click <a href=\"/LED=OFF\">here</a> turn the LED on pin 2 OFF<br>");
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}
