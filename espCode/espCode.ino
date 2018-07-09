
#define DEBUG_ESP_PORT = Serial;

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ESP8266Ping.h>
#include "Queue.h"
#include <EEPROM.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <TaskScheduler.h>
#include <ArduinoLog.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int EEPROM_FIRST_BOOT_STATUS = 0;
const int EEPROM_SSID_START = 1;
const int EEPROM_SSID_STOP = 64;
const int EEPROM_WIFIPASS_START = 65;
const int EEPROM_WIFIPASS_STOP = 128;
const int EEPROM_USERNAME_START = 129;
const int EEPROM_USERNAME_STOP = 192;
const int EEPROM_USERPASS_START = 193;
const int EEPROM_USERPASS_STOP = 256;
const int EEPROM_DEVICENAME_START = 257;
const int EEPROM_DEVICENAME_STOP = 320;
const int EEPROM_MODE = 321; //  0 for setup mode or 1 for connect mode
const int EEPROM_RELAYSTATUS = 323;

const int RELAY_PIN = 2;
const int writeableEEPROMArea = 512;

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

const IPAddress apIP(192, 168, 1, 1);

DNSServer dnsServer;
ESP8266WebServer webServer(80);

String ssidList;
String ssid;
String pass;
String accountusername;
String accountpass;
String devicename;
String toMSBuffer;
String deviceid = "5ri09trsm5sil31ai5pa68ldh3";

const char* apSSID = "ESP8266_SETUP";
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *serialbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *outbuffer = "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char* mainserver = "192.168.11.120";

int bufferHeadindex = 0;
int opmode;
int relayStatus;

static unsigned long unixtime = 0;
static unsigned long tempunixtime = 0;

char c = 0;

bool connected;
bool timeIsSet;
bool restart;
bool mode;
bool connectedToWifi;
bool connectedToInternet;
bool connectedToMS;
bool settingMode;

void updateConnectionStatus();

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  
  switch(type) {
    //
    case WStype_ERROR:{
      connected = 0;
//      Serial.printf("[WSc] Error!: %s\n",  payload);
      Log.notice("[WSc] Error!: %s\n",  payload);
    }
    break;

    //
    case WStype_DISCONNECTED:{
      connected = 0;
      Log.notice("[WSc] Disconnected!\n");
//      Serial.printf("[WSc] Disconnected!\n");
    }
    break;

    //
    case WStype_CONNECTED:{
      Log.notice("[WSc] Connected!\n");
//      Serial.printf("[WSc] Connected!\n");
      connected = 1;
      registerDeviceWithMS();
//      reqTimeFromMS();
    }
    break;

    //
    case WStype_TEXT:{    
      Log.notice("[WSc] get text: %s\n", payload);
//      Serial.printf("[WSc] get text: %s\n", payload);
      StaticJsonBuffer<200> jsonBuffer;          
      JsonObject& root = jsonBuffer.parseObject((char *) payload);      
      const char* type = root["type"];

      if(strcmp(type, "currentTime") == 0){
        // ESP is working under the assumption that the cu needs the time as soon as possible
        // therefore rather than adding the currenttime message to queue it is delivered to the mega ASAP
//        sendCurrentTimeToCU(root["currentTime"]);
       
        root.printTo(Serial);
        
      }else if(strcmp(type, "restart") == 0){
        restart = 1;

      }else if(strcmp(type, "relayUpdate") == 0){
        relayStatus = root["relay"];
        Log.notice("Writing Relay Status to EEPROM: %d", relayStatus);
    
        EEPROM.write(EEPROM_RELAYSTATUS, relayStatus);
        EEPROM.commit();
        root.printTo(Serial);
               
      }else{      
        
      }
      
    }
    break;      
  }
}

// example JSON objects
// {"type":"readytosend"}
// {"type":"powerUpdate","timeStamp":"152788383","totalPower":"42"}
// {"type":"relayUpdate"}
// {"type":"restart","mode":"0"}
// {"type":"currentTime"}

// used to receive data from the cu
void readCU(){
  // NOTE: END OF LINE IS SPECIFIED WITH A NEW LINE CHARACTER
  // send data only when you receive data:
  if (Serial.available() > 0) {
    DynamicJsonBuffer jsonBuffer(512);
    JsonObject& root = jsonBuffer.parseObject(Serial);
    const char* type = root["type"];

    // check if passed string is in correct JSON format
    if (root.success()) {
      if(strcmp(type, "powerUpdate") == 0 ){
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root1 = jsonBuffer.createObject();
        root1["deviceid"] = deviceid;
        root1["type"] = "powerUpdate";
        root1["totalPower"] = root["totalPower"];
        root1["timeStamp"] = root["timeStamp"];
        
        // automatically relay power update and currentTime JSON objects
        String message = "";
        root1.printTo(message);
    
        message.toCharArray(outbuffer, 200);
        Log.notice("Sending: %s\n",outbuffer); 
        webSocket.sendTXT(outbuffer);
      }else if(strcmp(type, "currentTime") == 0){
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root1 = jsonBuffer.createObject();
        root1["deviceid"] = deviceid;
        root1["type"] = "currentTime";
        
        String message;
        root1.printTo(message);
        message.toCharArray(outbuffer, 100);
        Log.notice("Sending: %s\n",outbuffer);
        webSocket.sendTXT(outbuffer);
      }else if(strcmp(type, "restart") == 0){
        // cu is telling the ESP to restart
        restart = 1;
        opmode = root["mode"];
        EEPROM.write(EEPROM_MODE, opmode);
        EEPROM.commit();

      }else if(strcmp(type, "getMode") == 0){
        // cu wants to know the current mode -> mode can be 0 for setup mode or 1 for connect mode
              
      }else if(strcmp(type, "relayUpdate") == 0){
        // send the most recent relay status to the cu
//        sendRelayUpdateToCU(relayStatus);
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root1 = jsonBuffer.createObject();
        root1["deviceid"] = deviceid;
        root1["type"] = "relayUpdate";
        
        String message;
        root1.printTo(message);
        message.toCharArray(outbuffer, 100);
        Log.notice("Sending: %s\n",outbuffer);
        webSocket.sendTXT(outbuffer);
        
      }else if(strcmp(type, "tempUpdate") == 0){
        // send the most recent relay status to the cu
//        sendRelayUpdateToCU(relayStatus);
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root1 = jsonBuffer.createObject();
        root1["deviceid"] = deviceid;
        root1["type"] = "tempUpdate";
        root1["timeStamp"] = root["timeStamp"];
        root1["temp"] = root["temp"];
        
        String message;
        root1.printTo(message);
        message.toCharArray(outbuffer, 100);
        Log.notice("Sending: %s\n",outbuffer);
        webSocket.sendTXT(outbuffer);
        
      }else if(strcmp(type, "readytosend") == 0){//{"type":"readytosend"}
        // cu wants to know if the esp is connected to main server {"type":”readytosend”,”readytosend”:”1”}
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root = jsonBuffer.createObject();        
        root["type"] = "readytosend";
        root["readytosend"] = "1";
        root["mode"] = opmode;
        
//        if(connectedToInternet){
//          // connected to ms
//          root["readytosend"] = "1";
//        }else{
//          // not connected to ms
//          root["readytosend"] = "0";
//        }

        root.printTo(Serial);
        Log.notice("\n");
       
      }else{  
        
      }      
    }
           
  }else{

  }
}

// Sends the current time to the cu
void sendCurrentTimeToCU(int currentTime){
  int waitPeriod = 10;
  int i = 0;
  
  // wait a period for the serial to be available
  while(i < waitPeriod){
    if(Serial.available()){
      StaticJsonBuffer<200> jsonBuffer;  
      JsonObject& root = jsonBuffer.createObject();
      root["type"] = "currentTime";
      root["currentTime"] = currentTime;

      root.printTo(Serial);
      
      return;
    }else{
      delay(1000);
    }
    i = i + 1;
  }
  
}

// requests the current time from the MS
void reqTimeFromMS(){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "currentTime";
  root["accountusername"] = accountusername.c_str();
  root["accountpass"] = accountpass.c_str();

  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 400);
  Log.notice("Sending: %s\n", outbuffer);
  webSocket.sendTXT(outbuffer);
}

// requests the current time from the MS
void registerDeviceWithMS(){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "registerDevice";
  root["accountusername"] = accountusername.c_str();
  root["accountpass"] = accountpass.c_str();
  root["devicename"] = devicename.c_str();
  
  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 400);
  Log.notice("Sending: %s\n", outbuffer);
  webSocket.sendTXT(outbuffer);
}

// requests the current relay status from the MS
void reqRelayStatusFromMS(){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "relayUpdate";

  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Log.notice("Sending: %s\n", outbuffer);
  webSocket.sendTXT(outbuffer);
}

// sends the current mode of the ESP to the cu
void sendModeToCU(int mode){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["mode"] = mode;
  root["deviceid"] = deviceid;
  root["type"] = "mode";

  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Log.notice("%s\n",outbuffer); 
}

//
void sendPowerMeasurementToMS(int totalPower,int timeStamp){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "powerMeas";
  root["totalPower"] = String(totalPower);
  root["timeStamp"] = String(timeStamp);
  
//  String message;
//  root.printTo(message);
//  message.toCharArray(outbuffer, 100);
//  Serial.print("Sending: ");
//  Serial.println(outbuffer); 
//  webSocket.sendTXT(outbuffer);

  toMSBuffer = "";
  root.printTo(toMSBuffer);
  webSocket.sendTXT(toMSBuffer.c_str());
}

// Sends the current relay status to the cu
void sendRelayUpdateToCU(int status){
  
}

boolean checkInternetConnection(){
//  IPAddress ip (8, 8, 8, 8); // The remote ip to ping
//  bool ret = Ping.ping(ip);
  bool ret = Ping.ping("www.google.com", 1);
  return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


boolean restoreConfig() {
  opmode = EEPROM.read(EEPROM_MODE);
  Log.notice("opmode: %d", opmode);
  Log.notice("Reading EEPROM...");
//  String ssid = "";
//  String pass = "";
  ssid = "";
  pass = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Log.notice("SSID: ");
    Log.notice("%s\n",ssid.c_str());
    for (int i = 32; i < writeableEEPROMArea; ++i) {
      pass += char(EEPROM.read(i));
    }
    Log.notice("Password: ");
    Log.notice("%s\n",pass.c_str());
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    return true;
  }
  else {
    Log.notice("Config not found.");
    return false;
  }
}

void clearConfig(){
  for (int i = 0; i < writeableEEPROMArea; ++i) {
    EEPROM.write(i, 0);
    delay(25);
  }
  EEPROM.commit();
}

boolean checkConnection() {
  int count = 0;
  
  Log.notice("Checking Wi-Fi connection\n");
  while ( count < 30 ) {
    if (WiFiMulti.run() == WL_CONNECTED) {
      connectedToInternet = true;
      Log.notice("Connected!");
      return (true);
    }
    delay(500);
//    Log.notice(".");
    count++;
  }
  connectedToInternet = false;
  Log.notice("Timed out.");
  return false;
}



//void connectModeTest() {
//  Log.notice("Starting Web Server at ");
////    Serial.println(WiFi.localIP());
//    webServer.on("/", []() {
//      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
//      webServer.send(200, "text/html", makePage("STA mode", s));
//    });
//    webServer.on("/reset", []() {
//      for (int i = 0; i < writeableEEPROMArea; ++i) {
//        EEPROM.write(i, 0);
//      }
//      
//      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
//      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
//      
//      while(1){
//        delay(1000);
//        ESP.restart();
//      }
//      
//    });
//    webServer.begin();
//}

void connectModeSetup(){
  // a config already exists, check the wifi connection
    settingMode = false;
        
    connectedToWifi = checkConnection();

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(ssid.c_str(), pass.c_str());

    Log.notice("Reading EEPROM...\n");
    ssid = "";
    pass = "";
    accountusername = "";
    accountpass = "";
    devicename = "";

    if(opmode == 1) {  
      for (int i = 0; i < 32; ++i) {
        ssid += char(EEPROM.read(i));
      }
      Log.notice("SSID: %s\n",ssid.c_str());
      for (int i = 32; i < writeableEEPROMArea; ++i) {
        pass += char(EEPROM.read(i));
      }
      Log.notice("Password: %s\n",pass.c_str());
      
      for (int i = 0; i < 32; ++i) {
        accountusername += char(EEPROM.read(EEPROM_USERNAME_START+i));
      }
      Log.notice("Account Username: %s\n",accountusername.c_str());
      
      for (int i = 0; i < 32; ++i) {
        accountpass += char(EEPROM.read(EEPROM_USERPASS_START+i));
      }
      Log.notice("Account Password: %s\n",accountpass.c_str());

      for (int i = 0; i < 32; ++i) {
        devicename += char(EEPROM.read(EEPROM_DEVICENAME_START +i));
      }
      Log.notice("Device Name: %s\n",devicename.c_str());

    }

    WiFiMulti.addAP(ssid.c_str(), pass.c_str());
           
    pinMode(RELAY_PIN, OUTPUT);
    
    connectedToWifi = (WiFiMulti.run() == WL_CONNECTED);
    
    // establish a websocket connection with the main server
    if(connectedToWifi) {
      webSocket.beginSSL(mainserver, 443, "/wssdev", "");
      webSocket.onEvent(webSocketEvent);
      webSocket.setReconnectInterval(5000);
    }else{
      webSocket.beginSSL(mainserver, 443, "/wssdev", "");
      webSocket.onEvent(webSocketEvent);
      webSocket.setReconnectInterval(5000);
    }
    
}



void setupModeSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Log.notice("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
//  connectModeTest();

  Log.notice("Starting Web Server\n");
//  Serial.println(WiFi.softAPIP());
  WiFi.softAPIP();
  webServer.on("/settings", []() {
    String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
    s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
    s += ssidList;
//    s += "</select><br>Wifi-Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
    s += "</select><br>Wifi-Password: <input name=\"pass\" length=64 type=\"password\">";
    s += "</select><br>Device Name: <input name=\"devicename\" length=64>";
    s += "</select><br>Account Username: <input name=\"username\" length=64>";
    s += "</select><br>Account Password: <input name=\"userpass\" length=64 type=\"password\"><input type=\"submit\"></form>";
 
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });
  webServer.on("/setap", []() {
    for (int i = 0; i < writeableEEPROMArea; ++i) {
      EEPROM.write(i, 0);
    }
    String ssid = urlDecode(webServer.arg("ssid"));
    Log.notice("SSID: %s\n",ssid.c_str());
//    Log.notice("%s\n",ssid.c_str());
    String pass = urlDecode(webServer.arg("pass"));
    Log.notice("Password: %s\n",pass.c_str());
//    Log.notice("%s\n",pass.c_str());
    String accountusername = urlDecode(webServer.arg("username"));
    Log.notice("Account Username: %s\n",accountusername.c_str());
//    Log.notice("%s\n",accountusername.c_str());
    String accountpass = urlDecode(webServer.arg("userpass"));
    Log.notice("Account Password: %s\n",accountpass.c_str());
//    Log.notice("%s\n",accountpass.c_str());

    String devicename = urlDecode(webServer.arg("devicename"));
    Log.notice("Device Name: %s\n",devicename.c_str());
    
    Log.notice("Writing SSID to EEPROM...\n");
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(i, ssid[i]);
    }
    Log.notice("Writing Password to EEPROM...");
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(32 + i, pass[i]);
    }


    
    Log.notice("Writing Account Username to EEPROM...");
    for (int i = 0; i < accountusername.length(); i++) {
      EEPROM.write(i+EEPROM_USERNAME_START, accountusername[i]);
    }
    Log.notice("Writing Account Password to EEPROM...");
    for (int i = 0; i < accountpass.length(); i++) {
      EEPROM.write(i+EEPROM_USERPASS_START, accountpass[i]);
    }
    Log.notice("Writing Device Name to EEPROM...");
    for (int i = 0; i < devicename.length(); i++) {
      EEPROM.write(i+EEPROM_DEVICENAME_START , devicename[i]);
    }

    
    EEPROM.commit();
    Log.notice("Write EEPROM done!");
    String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
    s += ssid;
    s += "\" after the restart.";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    EEPROM.write(EEPROM_MODE, 1);
    delay(1000);
    EEPROM.commit();
    delay(1000);
    while(1){
      delay(1000);
      ESP.restart();
    }
  });
  webServer.onNotFound([]() {
    String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
    webServer.send(200, "text/html", makePage("AP mode", s));
  });
  webServer.begin();

  Log.notice("Starting Access Point at \"");
  Log.notice("%s\n",apSSID);
  Log.notice("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}



void updateConnectionStatus(){
  connectedToWifi = (WiFiMulti.run() == WL_CONNECTED);
  connectedToInternet = checkInternetConnection();
  if(connectedToInternet){
    Log.notice("Connected to internet");
  }else{
    Log.notice("No longer connected to internet");
  }
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  
  //    
  connected = 0;
  restart = 0;
  opmode = 0;
  
  //
  Serial.begin(9600);
  Serial.setDebugOutput(false);


//* 0 - LOG_LEVEL_SILENT     no output 
//* 1 - LOG_LEVEL_FATAL      fatal errors 
//* 2 - LOG_LEVEL_ERROR      all errors  
//* 3 - LOG_LEVEL_WARNING    errors, and warnings 
//* 4 - LOG_LEVEL_NOTICE     errors, warnings and notices 
//* 5 - LOG_LEVEL_TRACE      errors, warnings, notices & traces 
//* 6 - LOG_LEVEL_VERBOSE    all 
  Log.begin(LOG_LEVEL_SILENT , &Serial);

  connectedToWifi = 0;
  EEPROM.begin(writeableEEPROMArea);
  

//  clearConfig();
  
  delay(25);


  opmode = EEPROM.read(EEPROM_MODE);

  if (opmode == 1 && restoreConfig()) {
    relayStatus = EEPROM.read(EEPROM_RELAYSTATUS);
    connectModeSetup(); 
  }else{
    
    setupModeSetup();
  }
    
}

void loop() {

//    opmode = 0;
    // check for serial data from the cu
    readCU();
      
    // check if the ESP needs to restart
    if(restart){
      ESP.restart();
      Serial.flush();
    }

    if (opmode == 0) {
      // setup mode
      dnsServer.processNextRequest();
      webServer.handleClient();
    }else{
      WiFiMulti.run();
      webSocket.loop();
 
    }

}
