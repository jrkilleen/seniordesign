
#define DEBUG_ESP_PORT = Serial;

const int EEPROM_RELAYSTATUS = 0;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

Queue<String> mainServerInQueue = Queue<String>(10);
int RELAY_PIN = 2;
const int BUFFER_SIZE = 50;
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *serialbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *outbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *wifiID = "fbivan1";
char *wifiPass = "6aa4579140b89a323cd1e82e13a179e7a9f481bbc7ff4a8822b17ab684fe527b";
String userID = "testaccount";
String userPass = "testaccount";
String deviceid = "5ri09trsm5sil31ai5pa68ldh1";
char* mainserver = "192.168.11.120";
int bufferHeadindex = 0;
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
int opmode;
int relayStatus;
void updateConnectionStatus();
Scheduler runner;
Task t1(10000, TASK_FOREVER, &updateConnectionStatus);
//
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  
  switch(type) {
    //
    case WStype_ERROR:{
      connected = 0;
      Serial.printf("[WSc] Error!: %s\n",  payload);
    }
    break;

    //
    case WStype_DISCONNECTED:{
      connected = 0;
      Serial.printf("[WSc] Disconnected!\n");
    }
    break;

    //
    case WStype_CONNECTED:{
      Serial.printf("[WSc] Connected!\n");
      connected = 1;
      reqTimeFromMS();
    }
    break;

    //
    case WStype_TEXT:{    
      Serial.printf("[WSc] get text: %s\n", payload);
      StaticJsonBuffer<200> jsonBuffer;          
      JsonObject& root = jsonBuffer.parseObject((char *) payload);      
      const char* type = root["type"];

      if(strcmp(type, "currentTime") == 0){
        // ESP is working under the assumption that the cu needs the time as soon as possible
        // therefore rather than adding the currenttime message to queue it is delivered to the mega ASAP
        sendCurrentTimeToCU(root["currentTime"]);
      }else if(strcmp(type, "restart") == 0){
        restart = 1;

        
      }else if(strcmp(type, "relayUpdate") == 0){
          relayStatus = root["relay"];
               
      }else{      
        
      }
      
    }
    break;      
  }
}

// {"type":"readytosend"}
//{"type":"powerUpdate","timeStamp":"152788383","totalPower":"42"}
// {"type":"restart","mode":"0"}
// {"deviceid":"5ri09trsm5sil31ai5pa68ldh1","type":"currentTime"}
//example: {"deviceid":"5ri09trsm5sil31ai5pa68ldh1","type":"currentTime"}
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
        Serial.print("Sending: ");
        Serial.println(outbuffer); 
        webSocket.sendTXT(outbuffer);
      }else if(strcmp(type, "currentTime") == 0){
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root1 = jsonBuffer.createObject();
        root1["deviceid"] = deviceid;
        root1["type"] = "currentTime";
        
        String message;
        root1.printTo(message);
        message.toCharArray(outbuffer, 100);
        Serial.print("Sending: ");
        Serial.println(outbuffer); 
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
       
      }else if(strcmp(type, "readytosend") == 0){//{"type":"readytosend"}
        // cu wants to know if the esp is connected to main server {"type":”readytosend”,”readytosend”:”1”}
        StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root = jsonBuffer.createObject();        
        root["type"] = "readytosend";
        root["readytosend"] = "1";
        
        if(connectedToInternet){
          // connected to ms
          root["readytosend"] = "1";
        }else{
          // not connected to ms
          root["readytosend"] = "0";
        }

        root.printTo(Serial);
        Serial.print("\n");
       
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
 
      i = waitPeriod;
    }else{
      delay(1000);
    }
  }
  
}

// requests the current time from the MS
void reqTimeFromMS(){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "currentTime";

  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.print("Sending: ");
  Serial.println(outbuffer); 
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
  Serial.print("Sending: ");
  Serial.println(outbuffer); 
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
  Serial.println(outbuffer); 
}

//
void sendPowerMeasurementToMS(int totalPower,int timeStamp){
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["deviceid"] = deviceid;
  root["type"] = "powerMeas";
  root["totalPower"] = String(totalPower);
  root["timeStamp"] = String(timeStamp);
  
  String message;
  root.printTo(message);
  message.toCharArray(outbuffer, 100);
  Serial.print("Sending: ");
  Serial.println(outbuffer); 
  webSocket.sendTXT(outbuffer);
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


const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "ESP8266_SETUP";
boolean settingMode;
String ssidList;
const int writeableEEPROMArea = 1024;
  
DNSServer dnsServer;
ESP8266WebServer webServer(80);
String ssid;
String pass;
String accountusername;
String accountpass;

boolean restoreConfig() {
  opmode = EEPROM.read(EEPROM_MODE);
  Serial.print("opmode: ");
  Serial.println(opmode);
  Serial.println("Reading EEPROM...");
//  String ssid = "";
//  String pass = "";
  ssid = "";
  pass = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = 32; i < writeableEEPROMArea; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    return true;
  }
  else {
    Serial.println("Config not found.");
    return false;
  }
}

void clearConfig(){
  for (int i = 0; i < 256; ++i) {
    EEPROM.write(i, 0);
    delay(25);
  }
}

boolean checkConnection() {
  int count = 0;
  
  Serial.print("Checking Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFiMulti.run() == WL_CONNECTED) {
      connectedToInternet = true;
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  connectedToInternet = false;
  Serial.println("Timed out.");
  return false;
}



void connectModeTest() {
  Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    webServer.on("/", []() {
      String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("STA mode", s));
    });
    webServer.on("/reset", []() {
      for (int i = 0; i < writeableEEPROMArea; ++i) {
        EEPROM.write(i, 0);
      }
      
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      
      while(1){
        delay(1000);
        ESP.restart();
      }
      
    });
    webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
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

  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());
  webServer.on("/settings", []() {
    String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
    s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
    s += ssidList;
//    s += "</select><br>Wifi-Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
    s += "</select><br>Wifi-Password: <input name=\"pass\" length=64 type=\"password\">";
    s += "</select><br>Account Username: <input name=\"username\" length=64>";
    s += "</select><br>Account Password: <input name=\"userpass\" length=64 type=\"password\"><input type=\"submit\"></form>";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  });
  webServer.on("/setap", []() {
    for (int i = 0; i < writeableEEPROMArea; ++i) {
      EEPROM.write(i, 0);
    }
    String ssid = urlDecode(webServer.arg("ssid"));
    Serial.print("SSID: ");
    Serial.println(ssid);
    String pass = urlDecode(webServer.arg("pass"));
    Serial.print("Password: ");
    Serial.println(pass);
    String accountusername = urlDecode(webServer.arg("username"));
    Serial.print("Account Password: ");
    Serial.println(accountusername);
    String accountpass = urlDecode(webServer.arg("userpass"));
    Serial.print("Account Password: ");
    Serial.println(accountpass);
    Serial.println("Writing SSID to EEPROM...");
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(i, ssid[i]);
    }
    Serial.println("Writing Password to EEPROM...");
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(32 + i, pass[i]);
    }


    
    Serial.println("Writing Account Username to EEPROM...");
    for (int i = 0; i < accountusername.length(); i++) {
      EEPROM.write(i+EEPROM_USERNAME_START, accountusername[i]);
    }
    Serial.println("Writing Account Password to EEPROM...");
    for (int i = 0; i < accountpass.length(); i++) {
      EEPROM.write(i+EEPROM_USERPASS_START, accountpass[i]);
    }


    
    EEPROM.commit();
    Serial.println("Write EEPROM done!");
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

  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
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
    Serial.println("Connected to internet");
  }else{
    Serial.println("No longer connected to internet");
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
  
  //  
  connected = 0;
  restart = 0;
  relayStatus = 0;
  opmode = 0;
  
  //
  Serial.begin(9600);
  Serial.setDebugOutput(true);


  connectedToWifi = 0;
  EEPROM.begin(writeableEEPROMArea);
  
//  clearConfig();
  
  delay(10);
  opmode = EEPROM.read(EEPROM_MODE);
  if (opmode == 1 && restoreConfig()) {
    // a config already exists, check the wifi connection
    settingMode = false;
        
    connectedToWifi = checkConnection();
//    connectModeTest();
    WiFi.mode(WIFI_STA);
//    WiFiMulti.addAP("fbivan1", "6aa4579140b89a323cd1e82e13a179e7a9f481bbc7ff4a8822b17ab684fe527b");
    WiFiMulti.addAP(ssid.c_str(), pass.c_str());

    Serial.println("Reading EEPROM...");
    ssid = "";
    pass = "";
    accountusername = "";
    accountpass = "";
//    if (EEPROM.read(0) != 0) {
    if(opmode == 1) {  
      for (int i = 0; i < 32; ++i) {
        ssid += char(EEPROM.read(i));
      }
      Serial.print("SSID: ");
      Serial.println(ssid);
      for (int i = 32; i < writeableEEPROMArea; ++i) {
        pass += char(EEPROM.read(i));
      }
      Serial.print("Password: ");
      Serial.println(pass);
      
      for (int i = 0; i < 32; ++i) {
        accountusername += char(EEPROM.read(EEPROM_USERNAME_START+i));
      }
      Serial.print("Account Username: ");
      Serial.println(accountusername);
      
      for (int i = 0; i < 32; ++i) {
        accountpass += char(EEPROM.read(EEPROM_USERPASS_START+i));
      }
      Serial.print("Account Password: ");
      Serial.println(accountpass);

//      WiFi.setAutoReconnect(true);
//      WiFi.begin(ssid.c_str(), pass.c_str());

    }

//    WiFiMulti.addAP(wifiID, wifiPass);
    WiFiMulti.addAP(ssid.c_str(), pass.c_str());
    
    //
//    while(WiFiMulti.run() != WL_CONNECTED) {
//        delay(100);  
//    }
       
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
    
  }else{
    setupMode();
  }
  
  runner.init();  

  if(opmode == 1){
    runner.addTask(t1);
    delay(5000);
    t1.enable();
  }
  
}





void loop() {
    runner.execute();
    
      // check for serial data from the cu
    readCU();
    
    // check if the ESP needs to restart
    if(restart){
      ESP.restart();
    }

    if (opmode == 0) {
      // setup mode
      dnsServer.processNextRequest();
      webServer.handleClient();
    }else{
  
      webSocket.loop();
//      
//      // connect mode      
//      if(connectedToWifi) {
//        webSocket.loop(); 
//      }else{
//     
//      }  
    }



    //
  //  getSerialInput();
  
  //  if(relayStatus == 0){
  //    relayStatus = 1;
  //  }else{
  //    relayStatus = 0;
  //  }
  //  digitalWrite(RELAY_PIN, relayStatus);
  //  delay(1000);
    }
