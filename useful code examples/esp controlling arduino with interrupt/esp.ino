/*
 ESP8266 Blink by Simon Peter
 Blink the blue LED on the ESP-01 module
 This example code is in the public domain
 
 The blue LED on the ESP-01 module is connected to GPIO1 
 (which is also the TXD pin; so we cannot use Serial.print() at the same time)
 
 Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/
int pin = 2;// for esp01S can be 0 for gpio 0 or 2 for gpio 2
void setup() {
pinMode(pin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  
  digitalWrite(pin, 0);
  delay(5000);                      // Wait for two seconds (to demonstrate the active low LED)
  digitalWrite(pin, 1);
  delay(5000);  
}
