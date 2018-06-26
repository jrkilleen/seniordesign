
#define DEBUG_ESP_PORT = Serial;

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ESP8266Ping.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

const int BUFFER_SIZE = 50;
//char buffer[BUFFER_SIZE+1];
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *serialbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char *outbuffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

char c = 0;
int bufferHeadindex = 0;
static unsigned long unixtime = 0;
static unsigned long tempunixtime = 0;
String cookie = "9cumqa8qj596bfgpb6bfrko2f5";
char* mainserver = "192.168.11.120";
unsigned long average_ping_time_ms;


bool connected;
bool timeIsSet;
bool restart;

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
      StaticJsonBuffer<200> jsonBuffer;          
      JsonObject& root = jsonBuffer.parseObject((char *) payload);      
      const char* typef = root["type"];
      const char* msg = root["msg"];
      
      if(strcmp(msg, "time") == 0){
//        bool ret = Ping.ping(mainserver, 10);
//        average_ping_time_ms = Ping.averageTime();
        unixtime = root["seconds"];         
        unixtime = unixtime + average_ping_time_ms;  
        timeIsSet = 1; 
      }else if(strcmp(msg, "restart") == 0){
        restart = 1;
      }else{      
        Serial.printf("[WSc] get text: %s\n", msg);
      }
      
    }
    break;      
  }
}
















//
void reqTimeFromMS(){
  // get the average turn around time for transmission
  bool ret = Ping.ping(mainserver, 10);
  average_ping_time_ms = Ping.averageTime();
  
//  Serial.printf("ping avg time: %d\n", average_ping_time_ms);
  
//  String timerequest = "{\"cookie\":\"";
//  timerequest = timerequest + cookie;
//  timerequest = timerequest + "\",\"type\":\"cmd\",\"msg\":\"time\"}\n";
//  timerequest.toCharArray(buffer, 100);
//  Serial.printf(buffer);
//  webSocket.sendTXT(buffer);
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();
  root["cookie"] = cookie;
  root["type"] = "cmd";
  root["msg"] = "time";
  
  String timerequest;
  root.printTo(timerequest);
  timerequest.toCharArray(outbuffer, 100);
  Serial.print("Sending: ");
  Serial.println(outbuffer); 
  webSocket.sendTXT(outbuffer);
}

//
void getSerialInput(){
  // NOTE: END OF LINE IS SPECIFIED WITH A NEW LINE CHARACTER
      // send data only when you receive data:
  if (Serial.available() > 0) {
    
    // read the incoming char:
    c = Serial.read();
   
    if(c == '\n' || bufferHeadindex >= BUFFER_SIZE){
        serialbuffer[bufferHeadindex] = '\0';
        Serial.print("serial string received: ");
        Serial.println(serialbuffer);
//        String temp = serialbuffer;


//        String post = "{\"cookie\":\"";
//        post = post + cookie;
//        post = post + "\",\"type\":\"post\",\"msg\":\"";
//        post = post + serialbuffer;
//        post = post + "\"}\n";
//        post.toCharArray(outbuffer, 100);
 
         StaticJsonBuffer<200> jsonBuffer;  
        JsonObject& root = jsonBuffer.createObject();
        root["cookie"] = cookie;
        root["type"] = "post";
        root["msg"] = serialbuffer;
 
        String post;
        root.printTo(post);
        post.toCharArray(outbuffer, 100);
        Serial.print("Sending: ");
        Serial.println(outbuffer); 
        webSocket.sendTXT(outbuffer);
                
        bufferHeadindex = 0;       
        
    }else{
      serialbuffer[bufferHeadindex] = c;
      bufferHeadindex++;
//      Serial.print("serial received char: ");
//      Serial.println(c);
    }
            
  }else{

  }
}














void setup() {
  // USE_SERIAL.begin(921600);
  Serial.begin(9600);
  connected = 0;
  restart = 0;
  Serial.setDebugOutput(true);

//    for(uint8_t t = 4; t > 0; t--) {
//        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
//        Serial.flush();
//        delay(1000);
//    }


  WiFiMulti.addAP("fbivan1", "6aa4579140b89a323cd1e82e13a179e7a9f481bbc7ff4a8822b17ab684fe527b");

  //WiFi.disconnect();
  while(WiFiMulti.run() != WL_CONNECTED) {
      delay(100);  
  }
  
  webSocket.beginSSL(mainserver, 443, "/wss", "");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
     
}

void loop() {
  webSocket.loop(); 
  getSerialInput();

  // must be called within loop
  if(restart){
    ESP.restart();
  }
}















