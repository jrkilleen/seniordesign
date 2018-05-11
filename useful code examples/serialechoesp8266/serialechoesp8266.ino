// This is an esp8266 program. Characters sent to the esp over serial are stored in a buffer and echoed back to the sender. If the newline ('\n') character is received 
// then the stored buffer is sent from the esp to the sender and the buffer is cleared.

const int BUFFER_SIZE = 100;
char buffer[BUFFER_SIZE+1];
char c = 0;
int bufferHeadindex = 0;


void setup() {
    Serial.begin(9600);    
    Serial.print("Serial initialized\n");
}

void loop() {
  // send data only when you receive data:
  if (Serial.available() > 0) {
    
    // read the incoming char:
    c = Serial.read();
   
    if(c == '\n' || bufferHeadindex >= BUFFER_SIZE){
      
        buffer[++bufferHeadindex] = '\0';
        Serial.print("serial string received: ");
        //Serial.println(buffer);
        
        for(int i = 0; i < bufferHeadindex; i++){
          Serial.print(buffer[i]);
        }
        
        Serial.println();
        
        bufferHeadindex = 0;       
        
    }else{

      buffer[++bufferHeadindex] = c;
      Serial.print("serial received char: ");
      Serial.println(c);
    }
            
  }else{
    
  }

}
