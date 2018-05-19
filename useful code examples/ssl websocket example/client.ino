// modified code from https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp8266/WebSocketClientSSL/WebSocketClientSSL.ino



#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>

#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;


#define USE_SERIAL Serial

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {


    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            {
                USE_SERIAL.printf("[WSc] Connected to url: %s\n",  payload);
        
          // send message to server when Connected
        webSocket.sendTXT("Connected");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);

            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    }

}





const int BUFFER_SIZE = 50;
//char buffer[BUFFER_SIZE+1];
char *buffer = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
char c = 0;
int bufferHeadindex = 0;







void setup() {
    // USE_SERIAL.begin(921600);
    USE_SERIAL.begin(9600);

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

      for(uint8_t t = 4; t > 0; t--) {
          USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
          USE_SERIAL.flush();
          delay(1000);
      }


    WiFiMulti.addAP("ssid", "password");

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);  
    }

    webSocket.beginSSL("192.168.11.121", 8443);
    webSocket.onEvent(webSocketEvent);

}

void loop() {
    webSocket.loop(); 
    
    // send data only when you receive data:
  if (Serial.available() > 0) {
    
    // read the incoming char:
    c = Serial.read();
   
    if(c == '\n' || bufferHeadindex >= BUFFER_SIZE){
        buffer[bufferHeadindex] = '\0';
        Serial.print("serial string received: ");

        Serial.println(buffer);
        
//        for(int i = 0; i < bufferHeadindex; i++){
//          Serial.print(buffer[i]);
//          
//        }
        
        
        webSocket.sendTXT(buffer);
//        Serial.println();
        
        bufferHeadindex = 0;       
        
    }else{

      buffer[bufferHeadindex] = c;
      bufferHeadindex++;
//      Serial.print("serial received char: ");
//      Serial.println(c);
    }
            
  }else{
//    webSocket.sendTXT("secret code do not steal");
//    delay(1000);
  }
}