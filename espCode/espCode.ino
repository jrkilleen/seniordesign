
#define DEBUG_ESP_PORT = Serial;
#define MAX_EEPROM_STRING_SIZE = 2048;
#define EEPROM_STRING_BUFFER_SIZE = 1024;


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ESP8266Ping.h>
#include "Queue.h"
#include <EEPROM.h>

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
bool relayStatus;
bool talkingWithMS;
bool talkingWithMega;

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
      if(strcmp(type, "powerUpdate") == 0 || strcmp(type, "currentTime") == 0){
        // automatically relay power update and currentTime JSON objects
        String message;
        root.printTo(message);
        message.toCharArray(outbuffer, 100);
        Serial.print("Sending: ");
        Serial.println(outbuffer); 
        webSocket.sendTXT(outbuffer);
        
      }else if(strcmp(type, "restart") > 0){
        // cu is telling the ESP to restart
        restart = 1;
       
      }else if(strcmp(type, "getMode") > 0){
        // cu wants to know the current mode -> mode can be 0 for setup mode or 1 for connect mode
        
       
      }else if(strcmp(type, "relayUpdate") > 0){
        // send the most recent relay status to the cu
        sendRelayUpdateToCU(relayStatus);
       
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









void setup() {
  
  //  
  connected = 0;
  restart = 0;
  relayStatus = 0;
  
  //
  Serial.begin(9600);
  Serial.setDebugOutput(true);

//  WiFiMulti.addAP("fbivan1", "6aa4579140b89a323cd1e82e13a179e7a9f481bbc7ff4a8822b17ab684fe527b");
  WiFiMulti.addAP(wifiID, wifiPass);
  
  //
  while(WiFiMulti.run() != WL_CONNECTED) {
      delay(100);  
  }
  
  pinMode(RELAY_PIN, OUTPUT);
  
  // establish a websocket connection with the main server
  webSocket.beginSSL(mainserver, 443, "/wssdev", "");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  EEPROM.begin(512);
  int eeAddress = 0; 

  int f = 0;
  f = EEPROM.read(eeAddress);
  delay(500);
  Serial.printf("f = %d\n", f);
  f = f + 1;
  EEPROM.write(eeAddress, f);
  delay(500);
  EEPROM.commit();
}





void loop() {
  //
//  webSocket.loop(); 

  //
//  getSerialInput();

  // check if the ESP needs to restart
//  if(restart){
//    ESP.restart();
//  }

  // check for serial data from the cu
//  readCU();


  if(relayStatus == 0){
    relayStatus = 1;
  }else{
    relayStatus = 0;
  }
  digitalWrite(RELAY_PIN, relayStatus);
  delay(1000);

  
}

