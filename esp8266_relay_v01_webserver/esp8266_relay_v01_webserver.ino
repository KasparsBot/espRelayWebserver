/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-web-server-physical-button/
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.


---------------------------
  1) Change dummy inputs input1,input2,input3 to some real options
  3) do merge with EEPROM
TODO List:
  1) Change dummy inputs input1,input2,input3 to some real options
  1b) 
  2) Merge with TimeAlarm
  4) 
*********/

// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

//2021.04.07, X: EEPROM define START
#include <ESP_EEPROM.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
AlarmId Alarm_id;

//#include <NTPClient.h>
//#include <WiFiUdp.h>
//WiFiUDP ntpUDP;
//// By default 'pool.ntp.org' is used with 60 seconds update interval and
//// no offset
//NTPClient timeClient (ntpUDP);

//Library: https://github.com/SensorsIot/NTPtimeESP
#include <NTPtimeESP.h>
#define DEBUG_ON
NTPtime NTPserver("ch.pool.ntp.org");   // Choose server pool as required
strDateTime dateTime;

//WiFiServer server(80);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

struct day_time {
  byte h;
  byte m;
  byte s;
};

struct confGPIO {
  bool isUsed;
  byte pinNum;
  byte mode;
  bool defaultState;
};

struct confAlarm {
  int gpio;
  day_time start_time;
  int duration_sec;
  bool defaultState;
  int (*f)();
};

#define gpio_mode_INPUT INPUT
#define gpio_mode_OUTPUT OUTPUT
#define gpio_mode_INPUT_PULLUP INPUT_PULLUP

// The neatest way to access variables stored in EEPROM is using a structure
struct MyEEPROMStruct {
  int modificationNum;
  bool isAP;
  char ssid[32];
  char passwd[64];
  day_time start_time;
  day_time end_time;
  byte dataConfGPIO_count;
  confGPIO dataConfGPIO[2];
} eepromVar1, eepromVar2;

//2021.04.07, X: EEPROM define END

// Replace with your network credentials
const char* ssid = "ESP_WIFI";
const char* password = "12345678";

const char* PARAM_INPUT_1 = "state";
const char* PARAM_FORM_INPUT_1 = "input1";
const char* PARAM_FORM_INPUT_2 = "input2";
const char* PARAM_FORM_INPUT_3 = "input3";
char* PARAM_FORM_INPUT_VAL_1 = "00001111";

const char* webForm_ssid = "input_ssid";
const char* webForm_passwd = "input_passwd";
const char* webForm_led_start = "input_led_start";
const char* webForm_led_end = "input_led_end";
String webForm_input_tmp;


//const int output = 2;
const int output = D1;
const int buttonPin = D4;

// Variables will change:
int ledState = HIGH;          // the current state of the output pin
int buttonState;             // the current reading from the input pin
int buttonTriggState = LOW;
int buttonTriggStateLedAllow = HIGH;
bool buttonTriggOverrideState = false;
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 150;    // the debounce time; increase if the output flickers

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<style>
body {font-family: Arial;}

/* Style the tab */
.tab {
  overflow: hidden;
  border: 1px solid #ccc;
  background-color: #f1f1f1;
}

/* Style the buttons inside the tab */
.tab button {
  background-color: inherit;
  float: left;
  border: none;
  outline: none;
  cursor: pointer;
  padding: 14px 16px;
  transition: 0.3s;
  font-size: 17px;
}

/* Change background color of buttons on hover */
.tab button:hover {
  background-color: #ddd;
}

/* Create an active/current tablink class */
.tab button.active {
  background-color: #ccc;
}

/* Style the tab content */
.tabcontent {
  display: none;
  padding: 6px 12px;
  border: 1px solid #ccc;
  border-top: none;
}

/* Style the close button */
.topright {
  float: right;
  cursor: pointer;
  font-size: 28px;
}

.topright:hover {color: red;}
</style>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<!--
<h2>Tabs</h2>
<p>Click on the x button in the top right corner to close the current tab:</p>
-->
<div class="tab">
  <button class="tablinks" onclick="openCity(event, 'evtTabLight')" id="defaultOpen">Gaismu uzst&#257;d&#299;jumi</button>
  <button class="tablinks" onclick="openCity(event, 'evtTabWifi')">Wifi uzst&#257;d&#299;jumi</button>
</div>

<div id="evtTabLight" class="tabcontent">
  <span onclick="this.parentElement.style.display='none'" class="topright">&times</span>
  <!--
  <h3>London</h3>
  <p>London is the capital city of England.</p>
  -->
  %FORM_LIGHT_PLACEHOLDER%
</div>

<div id="evtTabWifi" class="tabcontent">
  <span onclick="this.parentElement.style.display='none'" class="topright">&times</span>
  <!--
  <h3>Paris</h3>
  <p>Paris is the capital of France.</p>
  -->
  %FORM_WIFI_PLACEHOLDER%
</div>

<!--
<h2>The name Attribute</h2>
<form action="/action_page.php">
  <label for="fname">First name:</label><br>
  <input type="text" id="fssid" value="SSID"><br><br>
  <input type="password" id="fpsswd" value="12345678"><br><br>
  <input type="submit" id="submitForm" value="Submit">
</form> 
-->
<script>

function openCity(evt, cityName) {
  var i, tabcontent, tablinks;
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.getElementById(cityName).style.display = "block";
  evt.currentTarget.className += " active";
}

// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click();

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

function togglePasswdType() {
  var x = document.getElementById("espFormInputPassword");
  if (x.type === "password") {
    x.type = "text";
  } else {
    x.type = "password";
  }
}

function submitForm() {
   alert("Submit button clicked!");
   return true;
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page

String outputState(){
  //if(digitalRead(output)){
  if(ledState){
    return "checked";
  }
  else {
    return "";
  }
  return "";
};

String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    if(outputStateValue = ledState ){
      //buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\">On</span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" checked" + outputStateValue + "><span class=\"slider\"></span></label>";
      buttons+= "<h4>Output - Gaisma Nr.1 - <span id=\"outputState\">Iesl&#275;gta</span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    } else {
      //buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\">Off</span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
      buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\">Izsl&#275;gta</span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" checked><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  if(var == "FORM_WIFI_PLACEHOLDER"){
    String forms ="";
    forms += " <form action=\"/submit\">";
/*
    forms += "input1: <input type=\"text\" name=\"input1\" value=\"" ;
    forms += String(PARAM_FORM_INPUT_VAL_1).c_str() ;
    forms += "\"><br>";
    forms += "input2: <input type=\"text\" name=\"input2\"><br>";
    forms += "input3: <input type=\"time\" name=\"input3\"><br>";
*/    
    forms += "&#160;&#160;&#160;&#160;&#160;&#160;&#160;"; //aligment spaces because leet html skilzz 
    forms += "T&#299;kls: <input type=\"text\" name=\"";
    forms += String(webForm_ssid).c_str();
    forms += "\" value=\"";
    forms += String(eepromVar1.ssid).c_str();
    forms += "\"><br>";

    forms += "&#160;&#160;&#160;&#160;";
    forms += "Parole: <input type=\"password\" name=\"";
    forms += String(webForm_passwd).c_str();
    forms += "\" id=\"espFormInputPassword\"> <br>";
    forms += "Show Password:<input type=\"checkbox\" onclick=\"togglePasswdType()\">";
    forms += "&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;<br><br>";
    forms += "<input type=\"submit\" value=\"Submit\">";
    forms += "</form><br>";
    //Serial.print(forms);
    return forms;
  }
  
  if(var == "FORM_LIGHT_PLACEHOLDER"){
    String forms ="";
    forms += " <form action=\"/submit\">";
/*
    forms += "input1: <input type=\"text\" name=\"input1\" value=\"" ;
    forms += String(PARAM_FORM_INPUT_VAL_1).c_str() ;
    forms += "\"><br>";
    forms += "input2: <input type=\"text\" name=\"input2\"><br>";
    forms += "input3: <input type=\"time\" name=\"input3\"><br>";
*/
    //forms += "Gaismu intervāls: <br>";
    forms += "Gaismu interv&#257;ls:&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;&#160;<br>";
    forms += "&#160;&#160;&#160;&#160;&#160;&#160;&#160; no: <input type=\"time\" name=\"";
    forms += String(webForm_led_start).c_str();
    forms += "\" value=\"";
    /*
    forms += String(eepromVar1.start_time.h).c_str();
    //forms += eepromVar1.start_time.h;
    forms += ":";
    //forms += eepromVar1.start_time.m;
    forms += String(eepromVar1.start_time.m).c_str();
    */
    forms += printWebFormTimeVal(eepromVar1.start_time);
    forms += "\"><br>";
    //Serial.printf("\n %i:%i", eepromVar1.start_time.h, eepromVar1.start_time.m);
    
    //forms += "&#160; līdz : <input type=\"time\" name=\"";
    forms += "&#160; l&#299;dz : <input type=\"time\" name=\"";
    forms += String(webForm_led_start).c_str();
    forms += "\" value=\"";
    /*
    //forms += eepromVar1.end_time.h;
    forms += String(eepromVar1.end_time.h).c_str();
    forms += ":";
    //forms += eepromVar1.end_time.m;
    forms += String(eepromVar1.end_time.m).c_str();
    */
    forms += printWebFormTimeVal(eepromVar1.end_time);
    forms += "\">";
    forms += "<br>";
    forms += "<br>";

    //forms += "&#160;";
    forms += "<input type=\"submit\" value=\"Submit\">";
    forms += "</form><br>";
    //Serial.print(forms);
    return forms;
  }
  return String();
}

void setup(){
  Alarm_id = Alarm.timerRepeat(2, Repeats2);
  
  // Serial port for debugging purposes
  Serial.begin(115200);
  /*
  eepromVar1.modificationNum = 56;
  eepromVar1.isAP = false;
  memcpy(eepromVar1.ssid, ssid, sizeof(eepromVar1.ssid));
  memcpy(eepromVar1.passwd, password, sizeof(eepromVar1.passwd));
  eepromVar1.start_time = set_day_time(806);
  eepromVar1.end_time = set_day_time(2030);
  
  eepromVar1.dataConfGPIO[0] = set_confGPIO(true, output, gpio_mode_OUTPUT, true);
  eepromVar1.dataConfGPIO[1] = set_confGPIO(true, buttonPin, gpio_mode_INPUT, true);
  eepromVar1.dataConfGPIO_count = 2;
  */
  
  EEPROM.begin(sizeof(MyEEPROMStruct));
  // Check if the EEPROM contains valid data from another run
  // If so, overwrite the 'default' values set up in our struct
  if(EEPROM.percentUsed()>=0) {
    EEPROM.get(0, eepromVar1);
    eepromVar1.modificationNum++;     // make a change to our copy of the EEPROM data
    
    Serial.printf("\nEEPROM has data from a previous run.");
    Serial.printf("\nESPROM used space: %d%%", EEPROM.percentUsed());
    Serial.printf("\nESPROM modification: %d", eepromVar1.modificationNum);

    //eepromVar1.dataConfGPIO[0] = set_confGPIO(pinLed, OUTPUT, true);
    //eepromVar1.dataConfGPIO[1] = set_confGPIO(pinLight, OUTPUT, true);
    
    //memcpy(eepromVar1.dataConfGPIO[0], set_confGPIO(pinLed, OUTPUT, false), sizeof(src[0])*len);
    //eepromVar1.dataConfGPIO_count = arrConfGPIO_count;
    Serial.printf("\nESPROM dataConfGPIO_count: %d", eepromVar1.dataConfGPIO_count);
    printConfGPIO(eepromVar1.dataConfGPIO, eepromVar1.dataConfGPIO_count);
    set_pinMode(eepromVar1.dataConfGPIO, eepromVar1.dataConfGPIO_count);

    //eepromVar1.isAP = true;//for testing forced AP mode
    if(!eepromVar1.isAP){
      Serial.println();
      Serial.print("Connecting to ");
      Serial.println(eepromVar1.ssid);
      WiFi.begin(eepromVar1.ssid, eepromVar1.passwd);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Alarm.delay(500);
        Serial.print(".");
        //TimerRepeats();
        yield();
        if(millis() > 45000 ){
          eepromVar1.isAP = true;
          break;
        }//if(millis() > 45000 )
      }//while (WiFi.status()
    printClock();
    Serial.print(": WiFi connected");
    }
    if(eepromVar1.isAP){
      //IPAddress    apIP(42, 42, 42, 42);  // Defining a static IP address: local & gateway
                                    // Default IP in AP mode is 192.168.4.1
      //IPAddress local_IP(192,168,4,22);
      //IPAddress gateway(192,168,4,9);
      //IPAddress subnet(255,255,255,0);
      
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00
      
      boolean result = WiFi.softAP(ssid, password);
      if(result == true)
      {
        printClock();
        Serial.print(":  WiFi SofAP connected, IP = ");
        Serial.println(WiFi.softAPIP());
      }
      else
      {
        printClock();
        Serial.print(":  WiFi SofAP failed!");
      }
    printClock();
    Serial.print(":  WiFi SofAP connected");
    Serial.print("Soft-AP IP address = ");
    Serial.println(WiFi.softAPIP());
    }


//    // Sync time with NTP server
//    timeClient.begin();
//    // Set offset time in seconds to adjust for your timezone, for example:
//    // GMT +1 = 3600
//    // GMT +8 = 28800
//    // GMT -1 = -3600
//    // GMT 0 = 0
//    timeClient.setTimeOffset(3600*3);
    Alarm_SyncNTPTime();

    // Start the webServer
    server.begin();
    printClock();
    Serial.print(": Server started: ");
    // Print the IP address
    Serial.print("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.print("/");
  } else {
    Serial.println("EEPROM size changed - EEPROM data zeroed - commit() to make permanent");    
  }  
  
  //2021.04.07, X: EEPROM setup END

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/action_page.php", HTTP_GET, [](AsyncWebServerRequest *request){
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      //digitalWrite(output, inputMessage.toInt());
      //ledState = !ledState;
      //Alarm_switchGpio(output, ledState);
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println();
    Serial.println(inputMessage);
    Serial.print(inputParam);
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/submit", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage = "";
    String inputParam;
    bool isEEPROMDataUpdated = false;

/*
  eepromVar1.modificationNum = 56;
  eepromVar1.isAP = false;
  memcpy(eepromVar1.ssid, ssid, sizeof(eepromVar1.ssid));
  memcpy(eepromVar1.passwd, password, sizeof(eepromVar1.passwd));
  eepromVar1.start_time = set_day_time(806);
  eepromVar1.end_time = set_day_time(2030);
  
  eepromVar1.dataConfGPIO[0] = set_confGPIO(true, output, gpio_mode_OUTPUT, true);
  eepromVar1.dataConfGPIO[1] = set_confGPIO(true, buttonPin, gpio_mode_INPUT, true);
  eepromVar1.dataConfGPIO_count = 2;
*/

    
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    /*
    if (request->hasParam(PARAM_FORM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_FORM_INPUT_1)->value();
      inputParam = PARAM_FORM_INPUT_1;
      //PARAM_FORM_INPUT_VAL_1 = inputMessage;
      inputMessage.toCharArray(PARAM_FORM_INPUT_VAL_1, 36);
      Serial.println(); Serial.print(inputParam); Serial.print(": "); Serial.print(inputMessage);
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    if (request->hasParam(PARAM_FORM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_FORM_INPUT_2)->value();
      inputParam = PARAM_FORM_INPUT_2;
      Serial.println(); Serial.print(inputParam); Serial.print(": "); Serial.print(inputMessage);
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    if (request->hasParam(PARAM_FORM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_FORM_INPUT_3)->value();
      inputParam = PARAM_FORM_INPUT_3;
      inputMessage.toCharArray(PARAM_FORM_INPUT_VAL_1, 36);
      Serial.println(); Serial.print(inputParam); Serial.print(": "); Serial.print(inputMessage);
    }
    */
    if (request->hasParam(webForm_ssid)) {
      inputMessage = request->getParam(webForm_ssid)->value();
      if(String(eepromVar1.ssid) != inputMessage){
        if(inputMessage == ""){
          eepromVar1.isAP = true;
        } else {
          eepromVar1.isAP = false;
        }
        inputMessage.toCharArray(eepromVar1.ssid, sizeof(eepromVar1.ssid));
        if(request->hasParam(webForm_passwd)){
          inputMessage = request->getParam(webForm_passwd)->value();
          Serial.println(); Serial.print("Password:"); Serial.print(inputMessage);
          inputMessage.toCharArray(eepromVar1.passwd, sizeof(eepromVar1.passwd));
        }
        isEEPROMDataUpdated = true;
      }//if(eepromVar1.ssid != inputMessage)
    }//if (request->hasParam(webForm_ssid))
    
    if(!isEEPROMDataUpdated && request->hasParam(webForm_passwd)){
      inputMessage = request->getParam(webForm_passwd)->value();
      Serial.println(); Serial.print("Password:"); Serial.print(inputMessage);
      inputMessage.toCharArray(eepromVar1.passwd, sizeof(eepromVar1.passwd));
    }
  
    
    if (request->hasParam(webForm_led_start)) {
      inputMessage = request->getParam(webForm_led_start)->value();
      //inputParam = PARAM_FORM_INPUT_3;
      Serial.println(); Serial.print(eepromVar1.start_time.h); Serial.print(" != "); Serial.print(inputMessage.substring(0, 2).toInt());
      if(eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()){
        Serial.println(); Serial.print("eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()");
        eepromVar1.start_time.h = inputMessage.substring(0, 2).toInt();
        isEEPROMDataUpdated = true;
      }      
      Serial.println(); Serial.print(eepromVar1.start_time.m); Serial.print(" != "); Serial.print(inputMessage.substring(3, 5).toInt());
      if(eepromVar1.start_time.m != inputMessage.substring(3, 5).toInt()){
        Serial.println(); Serial.print("eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()");
        eepromVar1.start_time.m = inputMessage.substring(3, 5).toInt();
        isEEPROMDataUpdated = true;
      }
      
      //inputMessage.toCharArray(PARAM_FORM_INPUT_VAL_1, 36);
      Serial.println(); Serial.print(inputParam); Serial.print(": "); Serial.print(inputMessage);
    }
    if (request->hasParam(webForm_led_end)) {
      inputMessage = request->getParam(webForm_led_end)->value();
      //inputParam = PARAM_FORM_INPUT_3;
      Serial.println(); Serial.print(eepromVar1.start_time.h); Serial.print(" != "); Serial.print(inputMessage.substring(0, 2).toInt());
      if(eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()){
        Serial.println(); Serial.print("eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()");
        eepromVar1.start_time.h = inputMessage.substring(0, 2).toInt();
        isEEPROMDataUpdated = true;
      }      
      Serial.println(); Serial.print(eepromVar1.start_time.m); Serial.print(" != "); Serial.print(inputMessage.substring(3, 5).toInt());
      if(eepromVar1.start_time.m != inputMessage.substring(3, 5).toInt()){
        Serial.println(); Serial.print("eepromVar1.start_time.h != inputMessage.substring(0, 2).toInt()");
        eepromVar1.start_time.m = inputMessage.substring(3, 5).toInt();
        isEEPROMDataUpdated = true;
      }
      
      //inputMessage.toCharArray(PARAM_FORM_INPUT_VAL_1, 36);
      Serial.println(); Serial.print(inputParam); Serial.print(": "); Serial.print(inputMessage);
    }
    
    if(inputMessage == "") {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    if(isEEPROMDataUpdated){
      //
      // (some code that might change the EEPROM data)
      // set the EEPROM data ready for writing
      EEPROM.put(0, eepromVar1);
      // write the data to EEPROM
      boolean ok = EEPROM.commit();
      Serial.println((ok) ? "Commit OK" : "Commit failed");
    
      // Get EEPROM data into our local copy
      // For this example, a different struct variable is used 
      EEPROM.get(0, eepromVar2);
    
      Serial.print("EEPROM data read, modificationNum=");
      Serial.println(eepromVar2.modificationNum);
    
      for (int i = 0; i < eepromVar2.dataConfGPIO_count; i++) {
        if(eepromVar2.dataConfGPIO[i].isUsed){
          Serial.println();
          Serial.printf("EEPROM data read, dataConfGPIO[%i].pinNum=", i);
          Serial.println(eepromVar2.dataConfGPIO[i].pinNum);
          
          //  Serial.print("EEPROM data read, dataConfGPIO[0].isUsed=");
          //  Serial.println((eepromVar2.dataConfGPIO[i].isUsed) ? "true" : "false");
          Serial.printf("EEPROM data read, dataConfGPIO[%i].mode=",i);
          Serial.println(eepromVar2.dataConfGPIO[i].mode);
          Serial.printf("EEPROM data read, dataConfGPIO[%i].defaultState=",i);
          Serial.println((eepromVar2.dataConfGPIO[i].defaultState) ? "true" : "false");
        }//if((eepromVar2.dataConfGPIO[i].isUsed)
      }//for (int i = 0; i < eepromVar2
    }//if(isEEPROMDataUpdated)
    
//    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
//                                     + inputParam + ") with value: " + inputMessage +
//                                     "<br><a href=\"/\">Return to Home Page</a>");
    request->send_P(200, "text/html", index_html, processor);
  });


  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      //digitalWrite(output, inputMessage.toInt());
      ledState = !ledState;
      Alarm_switchGpio(output, ledState);
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/state
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    //request->send(200, "text/plain", String(digitalRead(output)).c_str());
    request->send(200, "text/plain", String(ledState).c_str());
    Serial.println();
    Serial.print("send(200, text/plain,");
    Serial.print(String(digitalRead(output)).c_str());
    Serial.print("||");
    Serial.print(String(ledState).c_str());
    Serial.print(");");
  });
  
  // Start server 
  server.begin();

  buttonState = digitalRead(buttonPin);
  for(int i=0 ; i < 10; i++){
    
    if(buttonState != digitalRead(buttonPin)){
      i=0;
    }
  }//for(int i=0 ; i < 10; i++)
  

  // create the alarms, to trigger at specific times
  //Alarm.alarmRepeat(timeClient.getHours(),timeClient.getMinutes()+1,00, Alarm_Light_On);
  //Alarm.alarmRepeat(timeClient.getHours(),timeClient.getMinutes()+6,00, Alarm_Light_Off);
  //Alarm.alarmRepeat(8,30,0, Alarm_Light_On);  // 8:30am every day
  //Alarm.alarmRepeat(20,30,0, Alarm_Light_Off);  // 5:45pm every day
  //eepromVar1.start_time = set_day_time (210000);
  //eepromVar1.end_time = set_day_time (230000);
  printClock();
  Serial.printf(": Is Time between START_Time and END_Time( %d, %d, %d)"
    , int(hour()*10000 + minute()*100 + second())
    , int(eepromVar1.start_time.h*10000 + eepromVar1.start_time.m*100 + eepromVar1.start_time.s)
    , int(eepromVar1.end_time.h*10000 + eepromVar1.end_time.m*100 + eepromVar1.end_time.s)
    );
//  Serial.print("start_time:");
//  Serial.println(int(eepromVar1.start_time.h*10000 + eepromVar1.start_time.m*100 + eepromVar1.start_time.s));
//  Serial.print("end_time:");
//  Serial.println(int(eepromVar1.end_time.h*10000 + eepromVar1.end_time.m*100 + eepromVar1.end_time.s));
//  Serial.print("time:");
//  Serial.print(int(hour()*10000 + minute()*100 + second()));

  //Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm);  // 8:30:30 every Saturday
  // create timers, to trigger relative to when they're created
  //Alarm.timerRepeat(15, Repeats);           // timer for every 15 seconds
  //id = Alarm.timerRepeat(2, Repeats2);      // timer for every 2 seconds
  //Alarm.timerOnce(10, OnceOnly);            // called once after 10 seconds
  Alarm.alarmRepeat(eepromVar1.start_time.h, eepromVar1.start_time.m, 0, Alarm_Light_On);  // 8:30am every day
  Alarm.alarmRepeat(eepromVar1.end_time.h, eepromVar1.end_time.m, 0, Alarm_Light_Off);  // 5:45pm every day
  if(int(hour()*10000 + minute()*100 + second()) >= int(eepromVar1.start_time.h*10000 + eepromVar1.start_time.m*100 + eepromVar1.start_time.s)
    && int(hour()*10000 + minute()*100 + second()) < int(eepromVar1.end_time.h*10000 + eepromVar1.end_time.m*100 + eepromVar1.end_time.s)
  ){
    Alarm_Light_On();
  }
}
  
void loop() {
//  if(eepromVar1.isAP){
//    printClock();
//    Serial.printf(": Stations connected = %d\n", WiFi.softAPgetStationNum());
//  }
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      Serial.println(); Serial.print("if (reading != buttonState): ");
      //Serial.print(reading ? "true" : "false" );
      Serial.print(reading);
      Serial.print(" != ");
      //Serial.print(buttonState ? "true)" : "false)" );
      Serial.print(buttonState);
      
      buttonState = reading;
      
      // only toggle the LED if the new button state is HIGH
      if (buttonState == buttonTriggState) {
        Serial.println(); Serial.print("if (buttonState == buttonTriggState): (");
        Serial.print(buttonState ? "true" : "false" );
        Serial.print(" == ");
        Serial.print(buttonTriggState ? "true)" : "false)" );
        //ledState = !ledState;
        Serial.println(); Serial.print("Turn led ON:true");
        Alarm_switchGpio(output, true);
        buttonTriggOverrideState = true;
      } else {
        if(buttonTriggOverrideState){
          Serial.println(); Serial.print("if (buttonTriggOverride)");
          buttonTriggOverrideState = false;
          Alarm_switchGpio(output, ledState);
        }//if(buttonTriggOverrideState)
      }
    }//if (reading != buttonState)
  }//if ((millis() - lastDebounceTime) > debounceDelay)

  // set the LED:
  //digitalWrite(output, ledState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

void Alarm_switchGpio(const int inGpio, const bool inState){
  //ledState = !ledState;
  digitalWrite(inGpio, inState);
  
//  for ( int i=0; i < 18 ; i++){
//    if(i ==D1 || i ==D2 || i==D3 ||i==D4||i==D5||i==D6||i==D7||i==D8){
//      //pinMode(i, OUTPUT);
//      if(i != buttonPin){
//        digitalWrite(i, inState);
//      }
//    }
//  }


  
  //lastButtonState = inState;
  lastDebounceTime = millis();

 ledState = inState;          // the current state of the output pin
 //buttonState ;             // the current reading from the input pin
 //lastButtonState = inState;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
 lastDebounceTime = millis();  // the last time the output pin was toggled
//  printClock();
  Serial.printf(": Alarm_switchGpio(%i,", inGpio);
  Serial.print((inState) ? "true)" : "false)");
  //yield();        
};

//===================================================================
//
day_time set_day_time (const int inTime){
  day_time outTime;
  if(inTime <= 24 ){
    outTime.h = inTime;
    outTime.m = 0;
    outTime.s = 0;
  } else if (inTime <= 2459){
    outTime.h = inTime/100;
    outTime.m = inTime%100;
    if(outTime.m > 59){
      outTime.h++;
      outTime.m = outTime.m%60;
    }
    outTime.s = 0;
  } else if (inTime <= 245959){
    outTime.h = inTime/10000;
    outTime.m = (inTime/100)%100;
    outTime.s = inTime%100;
    if(outTime.s > 59){
      outTime.m++;
      outTime.s = outTime.s%60;
    }
    if(outTime.m > 59){
      outTime.h++;
      outTime.m = outTime.m%60;
    }
  } else {
    outTime.h = 0;
    outTime.m = 0;
    outTime.s = 0;
  }
  return outTime;
};

confGPIO set_confGPIO (const bool inIsUsed, const byte inPinNum, const byte inMode, const bool defaultState){
  confGPIO outConfGPIO;
  outConfGPIO.isUsed = inIsUsed;
  outConfGPIO.pinNum = inPinNum;
  outConfGPIO.mode = inMode;
  outConfGPIO.defaultState = defaultState;
  return outConfGPIO;
};

String printWebFormTimeVal(day_time inTime){
  String outTime = "";

  outTime += char(inTime.h/10 + 48);
  outTime += char(inTime.h%10 + 48);
  outTime += ':';
  outTime += char(inTime.m/10 + 48);
  outTime += char(inTime.m%10 + 48);
  //outTime[5] = '\0';
  
  //Serial.println("");
  //Serial.print("printMyTime: ");
  //Serial.print(outTime); // Do not remove, this somehow stores, or terminates char array making it acceessable for funct:"processor"

  return outTime;
};
//-------------------------------------------------------
void set_pinMode(confGPIO arrConfGPIO[], int arrConfGPIO_count){
  for (int i = 0; i < arrConfGPIO_count; i++) {
    printClock();
    Serial.printf("arrConfGPIO[%i].isUsed = %o", i, arrConfGPIO[i].isUsed);
    if(arrConfGPIO[i].isUsed){
      printClock();
      Serial.printf("pinMode(%i, %i, %o);", arrConfGPIO[i].pinNum, arrConfGPIO[i].mode, arrConfGPIO[i].defaultState);
      
      pinMode(arrConfGPIO[i].pinNum, arrConfGPIO[i].mode);
      if(arrConfGPIO[i].mode == OUTPUT){
        Alarm_switchGpio(arrConfGPIO[i].pinNum, arrConfGPIO[i].defaultState);
      }
    }//if(arrConfGPIO[i].isUsed)
  }//for (int i = 0; i < arrConfGPIO_count; i++)
};
//-------------------------------------------------------
void printConfGPIO(confGPIO inArrConfGPIO[], int inArrConfGPIO_count){
   for (int i = 0; i < inArrConfGPIO_count; i++) {
      if(eepromVar1.dataConfGPIO[i].isUsed){
        Serial.printf("\nEEPROM data read, dataConfGPIO[%i](", i);
        Serial.print((inArrConfGPIO[i].isUsed) ? "true, " : "false, ");
        Serial.printf("%i, ", inArrConfGPIO[i].pinNum);
        //Serial.printf("%i, %i, ", inArrConfGPIO[i].pinNum, inArrConfGPIO[i].mode);
        if(inArrConfGPIO[i].mode == OUTPUT){
          Serial.print("OUTPUT, ");
        } else if (inArrConfGPIO[i].mode == INPUT) {
          Serial.print("INPUT, ");
        } else if (inArrConfGPIO[i].mode == INPUT_PULLUP) {
          Serial.print("INPUT_PULLUP, ");
        } else {
          Serial.print("UNKNOWN, ");
        }
        Serial.print((inArrConfGPIO[i].defaultState) ? "true)" : "false)");
      }//if((eepromVar2.dataConfGPIO[i].isUsed)
    }//for (int i = 0; i < eepromVar2
}//void printConfGPIO
//=======================================================
void printClock() {
  // digital clock display of the time
  Serial.println();
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
//  Serial.print(" & ");
//  Serial.print(timeClient.getFormattedTime());
//  Serial.print(": ");
}
//-------------------------------------------------------
void digitalClockDisplay() {
  // digital clock display of the time
  printClock();
  Serial.println();
}

void Repeats2() {
  printClock();
  Serial.print(": 2 second timer");
  ledState = !ledState;
  Alarm_switchGpio(output, ledState);
}

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
void Alarm_SyncNTPTime(){
  //timeClient.update();
  printClock();
  Serial.print(": Alarm_SyncNTPTime()");
  dateTime = NTPserver.getNTPtime(2.0, 1);
  
  while(!dateTime.valid){
    Serial.print(".");
    dateTime = NTPserver.getNTPtime(2.0, 1);
//    Serial.println();
//    Serial.print("dateTime.hour: ");   
//    Serial.println(dateTime.hour);
//    Serial.print("dateTime.minute: ");   
//    Serial.println(dateTime.minute);
//    Serial.print("dateTime.second: "); 
//    Serial.println(dateTime.second);
//    Serial.print("dateTime.year: ");   
//    Serial.println(dateTime.year);
//    Serial.print("dateTime.month: ");   
//    Serial.println(dateTime.month);
//    Serial.print("dateTime.day: ");   
//    Serial.println(dateTime.day);
//    Serial.print("dateTime.dayofWeek: ");   
//    Serial.println(dateTime.dayofWeek);
    delay(1000);
  }
    printClock();
    Serial.print(": ");
    NTPserver.printDateTime(dateTime);
    setTime(dateTime.hour, dateTime.minute, dateTime.second, dateTime.day, dateTime.month, dateTime.year);
};

void Alarm_Light_On(){
  printClock();
  Serial.print(": Alarm: - turn lights ON");
  Alarm_switchGpio(output, false);
}
//-------------------------------------------------------
void Alarm_Light_Off(){
  printClock();
  Serial.print(": Alarm: - turn lights OFF");
  Alarm_switchGpio(output, true);
}
